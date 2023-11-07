#include "Bvh.h"

void Bvh::Generate(const std::vector<glm::vec3>& srcVertices, const std::vector<uint32_t>& srcTriangles) {

	if (srcVertices.size() == 0) return;

	stack.clear();
	triangles.clear();

	triangles.reserve(srcTriangles.size());

	// Remove indirection at the cost of an additional buffer
	for (int i = 0; i < srcTriangles.size(); i += 3) {

		const auto& v0 = srcVertices.data()[srcTriangles.data()[i]];
		const auto& v1 = srcVertices.data()[srcTriangles.data()[i + 1]];
		const auto& v2 = srcVertices.data()[srcTriangles.data()[i + 2]];

		triangles.push_back(BvhTriangle{
			.v0 = v0, .v1 = v1, .v2 = v2,
			.normal = glm::normalize(glm::cross(v0 - v1, v0 - v2)),
			.originalIndex = i,
		});
	}

	// Generate root
	BvhNode root;
	root.SetLeftIndex(0);
	root.SetRightIndex((int)triangles.size());
	CalculateNodeAABB(root);

	stack.push_back(root);

	SplitNode(0);
}

void Bvh::SplitNode(const int& nodeIdx) {

	auto& node = stack[nodeIdx];
	const auto aabbSize = node.aabb.Size();

#if true // SAH Splits

	int minAxis = 0;
	auto minSplitPos = node.aabb.Center();
	float minCost = std::numeric_limits<float>::max();
	const float invNodeArea = 1.0f / node.aabb.AreaHeuristic();

	// Naive = 22.5ms, 6 = 19.7ms, 10 = 19.6ms, 20 = 19.4ms, 100 = 19.0ms
	const float stepsize = 1.0f / 6.0f;

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

	const auto leftStackIndex = (int)stack.size();
	const auto rightStackIndex = (int)stack.size() + 1;

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

int Bvh::Partition(const int& low, const int& high, const glm::vec3& splitPos, const int& axis) {
	int pt = low;
	for (int i = low; i < high; i++)
		if (triangles[i].Max()[axis] < splitPos[axis])
			std::swap(triangles[i], triangles[pt++]); // Swap index i and pt in triangles array
	return pt;
}

AABB Bvh::CalculateAABB(const int& left, const int& right) const {
	AABB ret = AABB(triangles[left].v0);
	for (int i = left; i < right; i++) {
		ret.Encapsulate(triangles[i].v0);
		ret.Encapsulate(triangles[i].v1);
		ret.Encapsulate(triangles[i].v2);
	}
	return ret;
}

void Bvh::CalculateNodeAABB(BvhNode& node) {
	node.aabb = AABB(triangles[node.GetLeftIndex()].v0);
	for (int i = node.GetLeftIndex(); i < node.GetRightIndex(); i++) {
		node.aabb.Encapsulate(triangles[i].v0);
		node.aabb.Encapsulate(triangles[i].v1);
		node.aabb.Encapsulate(triangles[i].v2);
	}
}

// Adapted from Inigo Quilez https://iquilezles.org/articles/
float Bvh::ray_tri_intersect(const glm::vec3& ro, const glm::vec3& rd, const BvhTriangle& tri) const {
	const glm::vec3 v1v0 = tri.v1 - tri.v0, v2v0 = tri.v2 - tri.v0, rov0 = ro - tri.v0;
	const glm::vec3 n = glm::cross(v1v0, v2v0);
	const glm::vec3 q = glm::cross(rov0, rd);
	const float d = 1.0f / glm::dot(n, rd);
	const float u = d * glm::dot(-q, v2v0);
	if (u < 0.0f) return -1.0f;
	const float v = d * glm::dot(q, v1v0);
	if (v < 0.0f) return -1.0f;
	if (u + v <= 1.0f) {
		return d * glm::dot(-n, rov0); // Doublesided
		//const float dd = glm::dot(-n, rov0);
		//if (dd < 0.0f) return d * dd; // Backface culling
	}
	return -1.0f;
}

float Bvh::Intersect(const Ray& ray, glm::vec3& normal, int& minIndex, float& depth) const {
	depth = 99999999.9f;
	minIndex = -1; // Overflows to max val
	IntersectNode(0, ray, normal, minIndex, depth);
	return minIndex != -1;
}

// Recursed func
void Bvh::IntersectNode(const int& nodeIndex, const Ray& ray, glm::vec3& normal, int& minTriIdx, float& minDist) const {

	const auto& node = stack[nodeIndex];

	// Leaf -> Check tris
	if (node.IsLeaf()) {
		for (int i = node.GetLeftIndex(); i < node.GetRightIndex(); i++) {

			const auto& tri = triangles[i];

			if (tri.originalIndex == ray.mask) continue;

			const auto res = ray_tri_intersect(ray.ro, ray.rd, tri);

			if (res > 0.0f && res < minDist) {
				minDist = res;
				minTriIdx = tri.originalIndex;
				normal = tri.normal;
			}
		}
		return;
	}

	// Node -> Check left/right node
	else {

		auto nodeA = node.GetLeftChild();
		auto nodeB = node.GetRightChild();
		auto intersectA = stack[nodeA].aabb.Intersect(ray);
		auto intersectB = stack[nodeB].aabb.Intersect(ray);

		// Swap A closer
		if (intersectB < intersectA) {
			std::swap(nodeA, nodeB);
			std::swap(intersectA, intersectB);
		}

		// Both either behind or we're inside -> check both
		if (intersectA < 0.0f && intersectB < 0.0f) {
			if (stack[nodeA].aabb.Contains(ray.ro))
				IntersectNode(nodeA, ray, normal, minTriIdx, minDist);
			if (stack[nodeB].aabb.Contains(ray.ro))
				IntersectNode(nodeB, ray, normal, minTriIdx, minDist);
		}
		// A behind or we're inside and B in front or miss
		else if (intersectA < 0.0f) {
			if (stack[nodeA].aabb.Contains(ray.ro))
				IntersectNode(nodeA, ray, normal, minTriIdx, minDist);
			if (intersectB != 0.0f && intersectB < minDist)
				IntersectNode(nodeB, ray, normal, minTriIdx, minDist);
		}
		// B behind or we're inside and A in front or miss
		else if (intersectB < 0.0f) {
			if (stack[nodeB].aabb.Contains(ray.ro))
				IntersectNode(nodeB, ray, normal, minTriIdx, minDist);
			if (intersectA != 0.0f && intersectA < minDist)
				IntersectNode(nodeA, ray, normal, minTriIdx, minDist);
		}
		else { // Both in front or missed, check closer first
			if (intersectA != 0.0f && intersectA < minDist)
				IntersectNode(nodeA, ray, normal, minTriIdx, minDist);
			if (intersectB != 0.0f && intersectB < minDist)
				IntersectNode(nodeB, ray, normal, minTriIdx, minDist);
		}
	}
}
