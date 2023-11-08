#include "BvhPoint.h"

#include <thread>
#include <ppl.h> // Parallel for

#include "Engine/Timer.h"
#include "Engine/Log.h"

void BvhPoint::Generate(const std::vector<glm::vec4>& srcPoints) {

	stack.clear();
	cc_stack.clear();
	points.clear();

	if (srcPoints.size() == 0) return;

	// Remove indirection at the cost of an additional buffer
	static_assert(sizeof(BvhPointData) == sizeof(glm::vec4));
	points.resize(srcPoints.size());
	memcpy(points.data(), srcPoints.data(), srcPoints.size() * sizeof(glm::vec4));

	// Generate root
	BvhNode root;
	root.SetLeftIndex(0);
	root.SetRightIndex((int)points.size());
	CalculateNodeAABB(root);
	cc_stack.push_back(root);
	//SplitNodeRecurse(0); // Single threaded

	
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

	// Copy to standard vector to speed up querying
	stack.assign(cc_stack.begin(), cc_stack.end());
}

void BvhPoint::SplitNodeSingle(const int& nodeIdx, int& nextLeft, int& nextRight) {

	auto& node = cc_stack[nodeIdx];
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

	const int leftStackIndex = (int)(cc_stack.push_back(BvhNode()) - cc_stack.begin());
	const int rightStackIndex = (int)(cc_stack.push_back(BvhNode()) - cc_stack.begin());
	//const auto leftStackIndex = (int)stack.size();
	//const auto rightStackIndex = (int)stack.size() + 1;

	left.SetLeftIndex(node.GetLeftIndex());
	left.SetRightIndex(splitPoint);

	right.SetLeftIndex(splitPoint);
	right.SetRightIndex(node.GetRightIndex());

	node.SetLeftChild(leftStackIndex);
	node.SetRightChild(rightStackIndex);

	CalculateNodeAABB(left);
	CalculateNodeAABB(right);

	cc_stack[leftStackIndex] = left;
	cc_stack[rightStackIndex] = right;
	//stack.push_back(left);
	//stack.push_back(right);

	if (left.ElementCount() > maxNodeEntries) nextLeft = leftStackIndex;
	if (right.ElementCount() > maxNodeEntries) nextRight = rightStackIndex;
}

void BvhPoint::SplitNodeRecurse(const int& nodeIdx) {
	int nextLeft = -1, nextRight = -1;
	SplitNodeSingle(nodeIdx, nextLeft, nextRight);
	if (nextLeft != -1) SplitNodeRecurse(nextLeft);
	if (nextRight != -1) SplitNodeRecurse(nextRight);
}

int BvhPoint::Partition(const int& low, const int& high, const glm::vec3& splitPos, const int& axis) {
	int pt = low;
	for (int i = low; i < high; i++)
		if (points[i].point[axis] < splitPos[axis])
			std::swap(points[i], points[pt++]); // Swap index i and pt in points array
	return pt;
}

AABB BvhPoint::CalculateAABB(const int& left, const int& right) const {
	AABB ret = AABB(points[left].point);
	for (int i = left + 1; i < right; i++)
		ret.Encapsulate(points[i].point);
	return ret;
}

void BvhPoint::CalculateNodeAABB(BvhNode& node) {
	node.aabb = AABB(points[node.GetLeftIndex()].point);
	for (int i = node.GetLeftIndex() + 1; i < node.GetRightIndex(); i++)
		node.aabb.Encapsulate(points[i].point);
}

// There's some repetition for 1, 3, 4 cases due to testing with quadrilaterals and triangles
// @TODO: Template to N case static array? might have perf traps

void BvhPoint::GetClosest(const glm::vec3& pos, const int& mask, float& dist, BvhPointData& d0) const {
	IntersectNode(0, pos, mask, dist, d0);
}

// Recursed func
void BvhPoint::IntersectNode(const int& nodeIndex, const glm::vec3& pos, const int& mask, float& minDist, BvhPointData& d0) const {

	const auto& node = stack[nodeIndex];

	// Leaf -> Check tris
	if (node.IsLeaf()) {
		for (int i = node.GetLeftIndex(); i < node.GetRightIndex(); i++) {
			const auto& pt = points[i];

			if ((pt.mask & mask) == 0) continue; // No lights of queried mask hit this

			const glm::vec3 os = (pt.point - pos);
			float dist = glm::dot(os, os);

			if (dist < minDist) {
				minDist = dist;
				d0 = pt;
			}
		}
		return;
	}

	// Node -> Check left/right node
	else {

		// Check if either hit
		auto nodeA = node.GetLeftChild();
		auto nodeB = node.GetRightChild();
		auto distA = stack[nodeA].aabb.SqrDist(pos);
		auto distB = stack[nodeB].aabb.SqrDist(pos);

		// Check closer first or else recursion becomes insanely slow
		if (distB < distA) {
			std::swap(distA, distB);
			std::swap(nodeA, nodeB);
		}
		
		if (distA < minDist)
			IntersectNode(nodeA, pos, mask, minDist, d0);
		if (distB < minDist)
			IntersectNode(nodeB, pos, mask, minDist, d0);
	}
}

// Returns 2 closest points and returns their dist and payload
void BvhPoint::Get3Closest(const glm::vec3& queryPos, const int& mask, glm::vec3& dists, BvhPointData& d0, BvhPointData& d1, BvhPointData& d2) const {
	Gather3Closest(0, queryPos, mask, dists, d0, d1, d2);
}

// Recursed func
void BvhPoint::Gather3Closest(const int& nodeIndex, const glm::vec3& pos, const int& mask, glm::vec3& dists, BvhPointData& d0, BvhPointData& d1, BvhPointData& d2) const {

	const auto& node = stack[nodeIndex];

	// Leaf -> Check tris
	if (node.IsLeaf()) {
		for (int i = node.GetLeftIndex(); i < node.GetRightIndex(); i++) {
			
			const auto& pt = points[i];

			if ((pt.mask & mask) == 0) continue; // No lights of queried mask hit this

			const glm::vec3 os = (pt.point - pos);
			const float dist = glm::dot(os, os);

			// Dist 2 = furthest
			if (dist < dists[2]) {
				if (dist < dists[1]) {
					if (dist < dists[0]) { // newdist = 0
						dists[2] = dists[1]; d2 = d1;
						dists[1] = dists[0]; d1 = d0;
						dists[0] = dist;	 d0 = pt;
					}
					else { // newdist = 1
						dists[2] = dists[1]; d2 = d1;
						dists[1] = dist;	 d1 = pt;
					}
				}
				else { // newdist = 2
					dists[2] = dist; d2 = pt;
				}
			}
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

		if (distA < dists[2])
			Gather3Closest(nodeA, pos, mask, dists, d0, d1, d2);
		
		if (distB < dists[2])
			Gather3Closest(nodeB, pos, mask, dists, d0, d1, d2);
	}
}


// Returns the 4 closest points to queryPos
void BvhPoint::Get4Closest(const glm::vec3& queryPos, const int& mask, glm::vec4& dists, 
	BvhPointData& d0, BvhPointData& d1, BvhPointData& d2, BvhPointData& d3) const {

	// Recursion seems faster than stack based search
	Gather4Closest(0, queryPos, mask, dists, d0, d1, d2, d3);
}

// Recursed func
void BvhPoint::Gather4Closest(const int& nodeIndex, const glm::vec3& pos, const int& mask, glm::vec4& dists, 
	BvhPointData& d0, BvhPointData& d1, BvhPointData& d2, BvhPointData& d3) const {

	const auto& node = stack[nodeIndex];

	// Leaf -> Check tris
	if (node.IsLeaf()) {
		for (int i = node.GetLeftIndex(); i < node.GetRightIndex(); i++) {

			const auto& pt = points[i];

			if ((pt.mask & mask) == 0) continue; // No lights of queried mask hit this

			const glm::vec3 os = (pt.point - pos);
			const float dist = glm::dot(os, os);

			// Dist 3 = furthest
			if (dist < dists[3]) {
				if (dist < dists[2]) {
					if (dist < dists[1]) {
						if (dist < dists[0]) { // newdist = 0
							dists[3] = dists[2]; d3 = d2;
							dists[2] = dists[1]; d2 = d1;
							dists[1] = dists[0]; d1 = d0;
							dists[0] = dist;	 d0 = pt;
						}
						else { // newdist = 1
							dists[3] = dists[2]; d3 = d2;
							dists[2] = dists[1]; d2 = d1;
							dists[1] = dist;	 d1 = pt;
						}
					}
					else { // newdist = 2
						dists[3] = dists[2]; d3 = d2;
						dists[2] = dist; d2 = pt;
					}
				}
				else { // newdist = 3
					dists[3] = dist; d3 = pt;
				}
			}
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

		if (distA < dists[3]) Gather4Closest(nodeA, pos, mask, dists, d0, d1, d2, d3);
		if (distB < dists[3]) Gather4Closest(nodeB, pos, mask, dists, d0, d1, d2, d3);
	}
}