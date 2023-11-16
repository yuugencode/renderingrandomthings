#include "BvhPoint.h"

#include <thread>
#include <ppl.h> // Parallel for

#include "Engine/Timer.h"
#include "Engine/Log.h"

#define MULTI_THREADED_GEN 1

template <typename T>
void BvhPoint<T>::Generate(const void* data, const int& count) {

	stack.clear();
	cc_stack.clear();
	points.clear();

	if (count == 0) return;

	// Remove indirection at the cost of an additional buffer
	points.resize(count);
	memcpy(points.data(), data, count * sizeof(BvhPointData));

	// Generate root
	BvhNode root;
	root.SetLeftIndex(0);
	root.SetRightIndex((int)points.size());
	root.aabb = CalculateAABB(root.GetLeftIndex(), root.GetRightIndex());

#if MULTI_THREADED_GEN
	cc_stack.push_back(root);

	// @TODO: Really need a bottom-up generation to replace this top-down one, but that needs a radix sort prepass for good query speeds
	// Top-down inherently doesn't parallelize well due to first pass always having to partition entire array
	const int numThreads = std::thread::hardware_concurrency();

	// Standard queue based multithreaded recursion
	queue.clear();
	queue.push(0);
	std::atomic_int cnt = 0;
	
	concurrency::parallel_for(0, numThreads, [&](size_t i) {
		int res = -1;
		while (true) {
			if (queue.try_pop(res)) {
				cnt++;
				int nl = -1, nr = -1;
				SplitNodeSingle(res, nl, nr);

				// Check if we're almost done and finish this root recursively without stressing queue if yes
				// If not, just add to queue
				if (nl != -1) {
					if (cc_stack[nl].ElementCount() < 128) SplitNodeRecurse(nl);
					else queue.push(nl);
				}
				if (nr != -1) {
					if (cc_stack[nr].ElementCount() < 128) SplitNodeRecurse(nr);
					else queue.push(nr);
				}

				cnt--;
			}
	
			// Cosmic level chance that a thread escapes but the algorithm works fine even with 1T
			if (queue.empty() && cnt == 0) break;
		}
	});

	// Copy to a standard vector to speed up querying
	stack.assign(cc_stack.begin(), cc_stack.end());
#else
	stack.push_back(root);
	SplitNodeRecurse(0); // Single threaded
#endif
}

template <typename T>
void BvhPoint<T>::Clear() {
	cc_stack.clear();
	points.clear();
	stack.clear();
}

template <typename T>
void BvhPoint<T>::SplitNodeSingle(const int& nodeIdx, int& nextLeft, int& nextRight) {

#if MULTI_THREADED_GEN
	auto& node = cc_stack[nodeIdx];
#else
	auto& node = stack[nodeIdx];
#endif
	const auto aabbSize = node.aabb.Size();

#if false // SAH Splits

	int minAxis = 0;
	auto minSplitPos = node.aabb.Center();
	float minCost = std::numeric_limits<float>::max();
	const float invNodeArea = 1.0f / node.aabb.AreaHeuristic();

	// Naive = 22.5ms, 6 = 19.7ms, 10 = 19.6ms, 20 = 19.4ms, 100 = 19.0ms
	const float stepsize = 1.0f / 3.0f;

	for (uint32_t axis = 0; axis < 3; axis++) {
		for (float t = stepsize; t < 1.0f; t += stepsize) {

			const glm::vec3 splitPos = node.aabb.min + aabbSize * t;
			const int splitPoint = Partition(node.GetLeftIndex(), node.GetRightIndex(), splitPos, axis);

			if (splitPoint == node.GetLeftIndex() || splitPoint == node.GetRightIndex())
				continue; // Couldn't split

			AABB aabbLeft = CalculateAABB(node.GetLeftIndex(), splitPoint);
			AABB aabbRight = CalculateAABB(splitPoint, node.GetRightIndex());

			int numRight = node.GetRightIndex() - splitPoint;
			int numLeft = splitPoint - node.GetLeftIndex();

			float cost = 1.0f + (numLeft * aabbLeft.AreaHeuristic() + numRight * aabbRight.AreaHeuristic()) * invNodeArea;

			if (cost < minCost) {
				minSplitPos = splitPos;
				minAxis = axis;
				minCost = cost;
			}
		}
	}

	int splitPoint = Partition(node.GetLeftIndex(), node.GetRightIndex(), minSplitPos, minAxis);

#else // Naive mid split

	uint32_t axis = 2;
	if (aabbSize.x > aabbSize.y && aabbSize.x > aabbSize.z) axis = 0;
	else if (aabbSize.y > aabbSize.z) axis = 1;

	const auto splitPos = node.aabb.Center();
	int splitPoint = Partition(node.GetLeftIndex(), node.GetRightIndex(), splitPos, axis);

	if (splitPoint == node.GetLeftIndex() || splitPoint == node.GetRightIndex()) {

		// Couldn't split the data any further, try the other 2 axes

		int axisA = (axis + 1) % 3, axisB = (axis + 2) % 3;

		if (aabbSize[axisA] > aabbSize[axisB]) {
			splitPoint = Partition(node.GetLeftIndex(), node.GetRightIndex(), splitPos, axisA);
			if (splitPoint == node.GetLeftIndex() || splitPoint == node.GetRightIndex())
				splitPoint = Partition(node.GetLeftIndex(), node.GetRightIndex(), splitPos, axisB);
		}
		else {
			splitPoint = Partition(node.GetLeftIndex(), node.GetRightIndex(), splitPos, axisB);
			if (splitPoint == node.GetLeftIndex() || splitPoint == node.GetRightIndex())
				splitPoint = Partition(node.GetLeftIndex(), node.GetRightIndex(), splitPos, axisA);
		}
	}
#endif

	if (splitPoint == node.GetLeftIndex() || splitPoint == node.GetRightIndex())
		return; // Can't split in any axis anymore, data is probably overlapping, just return

	// Generate new left / right nodes and split further
	BvhNode left, right;

#if MULTI_THREADED_GEN
	const int leftStackIndex = (int)(cc_stack.push_back(BvhNode()) - cc_stack.begin());
	const int rightStackIndex = (int)(cc_stack.push_back(BvhNode()) - cc_stack.begin());
#else
	const auto leftStackIndex = (int)stack.size();
	const auto rightStackIndex = (int)stack.size() + 1;
#endif

	left.SetLeftIndex(node.GetLeftIndex());
	left.SetRightIndex(splitPoint);

	right.SetLeftIndex(splitPoint);
	right.SetRightIndex(node.GetRightIndex());

	node.SetLeftChild(leftStackIndex);
	node.SetRightChild(rightStackIndex);

	left.aabb = CalculateAABB(left.GetLeftIndex(), left.GetRightIndex());
	right.aabb = CalculateAABB(right.GetLeftIndex(), right.GetRightIndex());

#if MULTI_THREADED_GEN
	cc_stack[leftStackIndex] = left;
	cc_stack[rightStackIndex] = right;
#else
	stack.push_back(left);
	stack.push_back(right);
#endif

	if (left.ElementCount() > maxNodeEntries) nextLeft = leftStackIndex;
	if (right.ElementCount() > maxNodeEntries) nextRight = rightStackIndex;
}

template <typename T>
void BvhPoint<T>::SplitNodeRecurse(const int& nodeIdx) {
	int nextLeft = -1, nextRight = -1;
	SplitNodeSingle(nodeIdx, nextLeft, nextRight);
	if (nextLeft != -1) SplitNodeRecurse(nextLeft);
	if (nextRight != -1) SplitNodeRecurse(nextRight);
}

//template <typename T>
//const glm::vec3& BvhPoint<T>::ReadPos(const int& i) const {
//	return *reinterpret_cast<glm::vec3*>(&points.data()[i]);
//}

template <typename T>
int BvhPoint<T>::Partition(const int& low, const int& high, const glm::vec3& splitPos, const int& axis) {
	int pt = low;
	for (int i = low; i < high; i++)
		if (points[i].point[axis] < splitPos[axis])
			std::swap(points[i], points[pt++]); // Swap index i and pt in points array
	return pt;
}

template <typename T>
AABB BvhPoint<T>::CalculateAABB(const int& left, const int& right) const {
	AABB ret = AABB(points[left].point);
	for (int i = left + 1; i < right; i++)
		ret.Encapsulate(points[i].point);
	return ret;
}

// Returns the 4 closest points to queryPos
template <typename T>
template <int N>
void BvhPoint<T>::GetNClosest(const glm::vec3& queryPos, float* dists, BvhPointData* data) const {
	static_assert(N > 0);
	// Recursion seems faster than stack based search
	GatherNClosest<N>(0, queryPos, dists, data);
}

// Recursed func
template <typename T> 
template <int N>
void BvhPoint<T>::GatherNClosest(const int& nodeIndex, const glm::vec3& pos, float* dists, BvhPointData* data) const {

	const auto& node = stack[nodeIndex];

	// Leaf -> Check tris
	if (node.IsLeaf()) {
		for (int i = node.GetLeftIndex(); i < node.GetRightIndex(); i++) {

			const auto& pt = points[i];
			const glm::vec3 os = (pt.point - pos);
			const float dist = glm::dot(os, os);

			if (dist > dists[N - 1]) continue;

			// Smaller than largest current, find spot, memmove current contents right and copy to position
			int j;
			for (j = N - 2; j >= 0; j--)
				if (dists[j] < dist)
					break;

			// Spot is j + 1
			memmove(&dists[j + 2], &dists[j + 1], (N - (j + 2)) * sizeof(float));
			memmove(&data[j + 2], &data[j + 1], (N - (j + 2)) * sizeof(BvhPointData));
			dists[j + 1] = dist;
			data[j + 1] = pt;
		}
		return;
	}

	// Node -> Check left/right node
	else {

		// Check if either hit
		auto nodeA = node.GetLeftChild();
		auto distA = stack[nodeA].aabb.SqrDist(pos);
		auto nodeB = node.GetRightChild();
		auto distB = stack[nodeB].aabb.SqrDist(pos);

		// Check closer first or else recursion becomes insanely slow
		if (distB < distA) {
			std::swap(distA, distB);
			std::swap(nodeA, nodeB);
		}

		if (distA < dists[N - 1]) GatherNClosest<N>(nodeA, pos, dists, data);
		if (distB < dists[N - 1]) GatherNClosest<N>(nodeB, pos, dists, data);
	}
}

// Have to explicit instantiate all used types
template class BvhPoint<float>;
template class BvhPoint<glm::vec3>;
template class BvhPoint<Empty>;

// Just add all 1-4 variants
template void BvhPoint<glm::vec3>::GetNClosest<1>(const glm::vec3& queryPos, float* dists, BvhPointData* data) const;
template void BvhPoint<glm::vec3>::GetNClosest<2>(const glm::vec3& queryPos, float* dists, BvhPointData* data) const;
template void BvhPoint<glm::vec3>::GetNClosest<3>(const glm::vec3& queryPos, float* dists, BvhPointData* data) const;
template void BvhPoint<glm::vec3>::GetNClosest<4>(const glm::vec3& queryPos, float* dists, BvhPointData* data) const;
template void BvhPoint<Empty>::GetNClosest<1>(const glm::vec3& queryPos, float* dists, BvhPointData* data) const;
template void BvhPoint<Empty>::GetNClosest<2>(const glm::vec3& queryPos, float* dists, BvhPointData* data) const;
template void BvhPoint<Empty>::GetNClosest<3>(const glm::vec3& queryPos, float* dists, BvhPointData* data) const;
template void BvhPoint<Empty>::GetNClosest<4>(const glm::vec3& queryPos, float* dists, BvhPointData* data) const;
