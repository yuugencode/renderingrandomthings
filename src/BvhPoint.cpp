#include "BvhPoint.h"

void BvhPoint::Generate(const std::vector<glm::vec3>& srcPoints) {

	if (srcPoints.size() == 0) return;

	stack.clear();
	points.clear();

	points.reserve(srcPoints.size());

	// Remove indirection at the cost of an additional buffer
	for (uint32_t i = 0; i < srcPoints.size(); i++) {
		points.push_back(BvhPointData{
			.point = srcPoints[i],
			.originalIndex = i
		});
	}

	// Generate root
	BvhNode root;
	root.SetLeftIndex(0);
	root.SetRightIndex((int)points.size());
	CalculateNodeAABB(root);

	stack.push_back(root);

	SplitNode(0);
}

void BvhPoint::SplitNode(const int& nodeIdx) {

	auto& node = stack[nodeIdx];
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

	const auto leftStackIndex = (uint32_t)stack.size();
	const auto rightStackIndex = (uint32_t)stack.size() + 1;

	left.SetLeftIndex(node.GetLeftIndex());
	left.SetRightIndex(splitPoint);

	right.SetLeftIndex(splitPoint);
	right.SetRightIndex(node.GetRightIndex());

	node.SetLeftChild(leftStackIndex);
	node.SetRightChild(rightStackIndex);

	CalculateNodeAABB(left);
	CalculateNodeAABB(right);

	stack.push_back(left);
	stack.push_back(right);

	if (left.TriangleCount() > maxNodeEntries) SplitNode(leftStackIndex);
	if (right.TriangleCount() > maxNodeEntries) SplitNode(rightStackIndex);
}

int BvhPoint::Partition(const int& low, const int& high, const glm::vec3& splitPos, const int& axis) {
	int pt = low;
	for (int i = low; i < high; i++)
		if (points[i].point[axis] < splitPos[axis])
			std::swap(points[i], points[pt++]); // Swap index i and pt in points array
	return pt;
}

AABB BvhPoint::CalculateAABB(const uint32_t& left, const uint32_t& right) const {
	AABB ret = AABB(points[left].point);
	for (uint32_t i = left + 1; i < right; i++)
		ret.Encapsulate(points[i].point);
	return ret;
}

void BvhPoint::CalculateNodeAABB(BvhNode& node) {
	node.aabb = AABB(points[node.GetLeftIndex()].point);
	for (int i = node.GetLeftIndex() + 1; i < node.GetRightIndex(); i++)
		node.aabb.Encapsulate(points[i].point);
}

float BvhPoint::GetClosest(const glm::vec3& pos, uint32_t& minIndex, float& dist) const {
	dist = 99999999.9f;
	minIndex = -1; // Overflows to max val
	IntersectNode(0, pos, minIndex, dist);
	return minIndex != -1;
}

// Recursed func
void BvhPoint::IntersectNode(const int& nodeIndex, const glm::vec3& pos, uint32_t& minIndex, float& minDist) const {

	const auto& node = stack[nodeIndex];

	// Leaf -> Check tris
	if (node.IsLeaf()) {
		for (int i = node.GetLeftIndex(); i < node.GetRightIndex(); i++) {
			const auto& pt = points[i];
			const glm::vec3 os = (pt.point - pos);
			float dist = glm::dot(os, os);

			if (dist < minDist) {
				minDist = dist;
				minIndex = pt.originalIndex;
			}
		}
		return;
	}

	// Node -> Check left/right node
	else {

		// Check if either hit
		const auto left = node.GetLeftChild();
		const auto right = node.GetRightChild();
		const auto distA = stack[left].aabb.SqrDist(pos);
		const auto distB = stack[right].aabb.SqrDist(pos);
		
		if (distA < minDist)
			IntersectNode(left, pos, minIndex, minDist);
		if (distB < minDist)
			IntersectNode(right, pos, minIndex, minDist);
	}
}

// Fills buffer with all points in given range
void BvhPoint::GetRange(const glm::vec3& pos, const float& range, std::vector<uint32_t>& buffer) const {
	GatherNode(0, range, pos, buffer);
}

// Recursed func
void BvhPoint::GatherNode(const int& nodeIndex, const float& range, const glm::vec3& pos, std::vector<uint32_t>& buffer) const {

	const auto& node = stack[nodeIndex];

	// Leaf -> Check tris
	if (node.IsLeaf()) {
		for (int i = node.GetLeftIndex(); i < node.GetRightIndex(); i++) {
			const auto& pt = points[i];
			const glm::vec3 os = (pt.point - pos);
			float dist = glm::dot(os, os);

			if (dist < range) buffer.push_back(pt.originalIndex);
		}
		return;
	}

	// Node -> Check left/right node
	else {

		// Check if either hit
		const auto left = node.GetLeftChild();
		const auto right = node.GetRightChild();
		const auto distA = stack[left].aabb.SqrDist(pos);
		const auto distB = stack[right].aabb.SqrDist(pos);

		if (distA < range)
			GatherNode(left, range, pos, buffer);
		if (distB < range)
			GatherNode(right, range, pos, buffer);
	}
}