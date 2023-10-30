module;

#include <glm/glm.hpp>

export module Bvh;

import <memory>;
import <vector>;
import Utils;
import Log;

export struct BvhNode {

	// Negative values = nonleaf, indices are to stack, encoded with -1 offset to avoid 0 index
	// Positive values = leaf, indices are to data, encoded as is
	int valL, valR;
public:

	BvhNode() {}

	AABB aabb;

	bool IsLeaf() const { return valL >= 0; }
	int GetLeftIndex() const { return valL; }
	int GetRightIndex() const { return valR; }
	int GetLeftChild() const { return -valL - 1; }
	int GetRightChild() const { return -valR - 1; }
	void SetLeftIndex(const int& val) { valL = val; }
	void SetRightIndex(const int& val) { valR = val; }
	void SetLeftChild(const int& val) { valL = -val - 1; }
	void SetRightChild(const int& val) { valR = -val - 1; }
	int TriangleCount() { return valR - valL; }
};

export class Bvh {
public:
	Bvh(){}

	// Abstraction for a single triangle
	struct BvhTriangle { 
		glm::vec3 v0, v1, v2; 
		glm::vec3 Centroid() const { return (v0 + v1 + v2) * 0.3333333333f; }
	};

	std::vector<BvhNode> stack;
	std::vector<BvhTriangle> triangles;

	static const int maxNodeEntries = 3;

	/// <summary> Generates a new BVH from given vertices/tris </summary>
	void Generate(const std::vector<glm::vec3>& srcVertices, const std::vector<uint32_t>& srcTriangles) {
		
		if (srcVertices.size() == 0) return;

		stack.clear();
		triangles.clear();

		triangles.reserve(srcTriangles.size());

		// Remove indirection at the cost of an additional buffer
		for (size_t i = 0; i < srcTriangles.size(); i += 3) {
			triangles.push_back(BvhTriangle{
				.v0 = srcVertices.data()[srcTriangles.data()[i]],
				.v1 = srcVertices.data()[srcTriangles.data()[i + 1]],
				.v2 = srcVertices.data()[srcTriangles.data()[i + 2]],
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

	void SplitNode(const int& nodeIdx) {

		auto& node = stack[nodeIdx];

		const auto splitPos = node.aabb.Center();
		const auto aabbSize = node.aabb.Size();

		// @IDEA: For the first ray traversal it might be possible to optimize building using camera information
		// for example partition using dot(viewdir, nrm) + dot(viewdir, ro - tripos) + (pos.x/y/z on largest_axis)
		// this should split backfacing triangles out very quick and prefer tris that are pointing at the cam
		// indirect bounces would have to use a standard bvh

		// Split naively on the largest aabb axis, @TODO: maybe SAH?
		int splitPoint;

		uint32_t axis = 2;
		if (aabbSize.x > aabbSize.y && aabbSize.x > aabbSize.z) axis = 0;
		else if (aabbSize.y > aabbSize.z) axis = 1;

		splitPoint = Partition(node.GetLeftIndex(), node.GetRightIndex(), splitPos, axis);

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

	/// Partitions data to 2 sides based on given pos and axis. Right index is exclusive.
	int Partition(const int& low, const int& high, const glm::vec3& splitPos, const int& axis) {
		int pt = low;
		for (int i = low; i < high; i++)
			if (triangles[i].Centroid()[axis] < splitPos[axis])
				std::swap(triangles[i], triangles[pt++]); // Swap index i and pt in triangles array
		return pt;
	}

	void CalculateNodeAABB(BvhNode& node) {
		node.aabb = AABB(triangles[node.GetLeftIndex()].v0);
		for (int i = node.GetLeftIndex(); i < node.GetRightIndex(); i++) {
			const auto& tri = triangles[i];
			node.aabb.Encapsulate(tri.v0);
			node.aabb.Encapsulate(tri.v1);
			node.aabb.Encapsulate(tri.v2);
		}
	}

	// Adapted from Inigo Quilez https://iquilezles.org/articles/
	float ray_tri_intersect(const glm::vec3& ro, const glm::vec3& rd, const BvhTriangle& tri) const {
		const auto v1v0 = tri.v1 - tri.v0, v2v0 = tri.v2 - tri.v0, rov0 = ro - tri.v0;
		const auto n = glm::cross(v1v0, v2v0);
		const auto q = glm::cross(rov0, rd);
		const auto d = 1.0f / glm::dot(n, rd);
		const auto u = d * glm::dot(-q, v2v0);
		if (u < 0.0f) return -1.0f;
		const auto v = d * glm::dot(q, v1v0);
		if (v < 0.0f) return -1.0f;
		if (u + v <= 1.0f) return d * glm::dot(-n, rov0);
		return -1.0f;
	}

	/// <summary> Intersects a ray against this bvh </summary>
	float Intersect(const glm::vec3& ro, const glm::vec3& rd, float& depth, int& minIndex) const {
		depth = 99999999.9f;
		minIndex = -1;
		IntersectNode(0, ro, rd, 1.0f / rd, minIndex, depth);
		return minIndex != -1;
	}

	// Recursed func
	void IntersectNode(const int& nodeIndex, const glm::vec3& ro, const glm::vec3& rd, const glm::vec3& inv_rd, int& minTriIdx, float& minDist) const {

		const auto& node = stack[nodeIndex];

		// Leaf -> Check tris
		if (node.IsLeaf()) {
			for (int i = node.GetLeftIndex(); i < node.GetRightIndex(); i++) {
				const auto res = ray_tri_intersect(ro, rd, triangles[i]);
				if (res > 0.0f && res < minDist) {
					minDist = res;
					minTriIdx = i;
				}
			}
			return;
		}

		// Node -> Check left/right node
		else {

			const auto left = node.GetLeftChild();
			const auto right = node.GetRightChild();
			const auto intersectA = stack[left].aabb.Intersect(ro, rd, inv_rd);
			const auto intersectB = stack[right].aabb.Intersect(ro, rd, inv_rd);

			// This nested if/else seems faster than swapping the values
			if (intersectA > 0.0f && intersectB > 0.0f) {
				if (intersectA < intersectB) {
					if (intersectA < minDist) IntersectNode(left, ro, rd, inv_rd, minTriIdx, minDist);
					if (intersectB < minDist) IntersectNode(right, ro, rd, inv_rd, minTriIdx, minDist);
				}
				else {
					if (intersectB < minDist) IntersectNode(right, ro, rd, inv_rd, minTriIdx, minDist);
					if (intersectA < minDist) IntersectNode(left, ro, rd, inv_rd, minTriIdx, minDist);
				}
			}
			else if (intersectA > 0.0f && intersectA < minDist)
				IntersectNode(left, ro, rd, inv_rd, minTriIdx, minDist);
			else if (intersectB > 0.0f && intersectB < minDist)
				IntersectNode(right, ro, rd, inv_rd, minTriIdx, minDist);
		}

	}
};

