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

	std::vector<BvhNode> stack;
	std::shared_ptr<std::vector<glm::vec3>> vertices;
	std::shared_ptr<std::vector<uint32_t>> triangleIndices;

	// Abstraction for triangles as 3 indices
	struct BvhTriangle { int v0i, v1i, v2i; };
	BvhTriangle* triangles() const { return (BvhTriangle*)triangleIndices->data(); }

	static const int maxNodeEntries = 20;

	/// <summary> Generates a new BVH from given vertices/tris </summary>
	void Generate(std::shared_ptr<std::vector<glm::vec3>> srcVertices, std::shared_ptr<std::vector<uint32_t>> srcTriangles) {
		
		this->vertices = srcVertices;
		this->triangleIndices = srcTriangles;

		if (vertices->size() == 0) return;

		stack.clear();

		// Generate root
		BvhNode root;
		root.aabb = AABB(vertices->size() != 0 ? vertices->at(0) : glm::vec3(0.0f, 0.0f, 0.0f));
		for (size_t i = 0; i < vertices->size(); i++)
			root.aabb.Encapsulate(vertices->at(i));

		root.SetLeftIndex(0);
		root.SetRightIndex((uint32_t)srcTriangles->size() / 3);
		stack.push_back(root);

		SplitNode(0);
	}

	void SplitNode(const int& nodeIdx) {

		auto& node = stack[nodeIdx];

		const auto splitPos = node.aabb.Center();
		const auto aabbSize = node.aabb.Size();

		// Split naively on the largest aabb axis, @TODO: SAH
		auto splitPoint = node.GetLeftIndex() + node.TriangleCount() / 2;

		if (aabbSize.x > aabbSize.y && aabbSize.x > aabbSize.z)
			splitPoint = Partition(node.GetLeftIndex(), node.GetRightIndex(), splitPos.x, 0);
		else if (aabbSize.y > aabbSize.z)
			splitPoint = Partition(node.GetLeftIndex(), node.GetRightIndex(), splitPos.y, 1);
		else
			splitPoint = Partition(node.GetLeftIndex(), node.GetRightIndex(), splitPos.z, 2);

		if (splitPoint == node.GetLeftIndex() || splitPoint == node.GetRightIndex())
			return; // Couldn't split the data any further

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

	/// <summary> Partitions data to 2 sides based on given pivot. RightIndex is EXCLUSIVE. </summary>
	int Partition(const int& low, const int& high, const float& pivot, const int& axis) {
		int pt = low;
		for (int i = low; i < high; i++) {
			
			const auto& arr = triangles();
			const auto& tri = arr[i];

			// Could precompute this centroid but maybe not worth?
			glm::vec3 centroid = (vertices->at(tri.v0i) + vertices->at(tri.v1i) + vertices->at(tri.v2i)) / 3.0f;

			if (centroid[axis] < pivot)
				std::swap(arr[i], arr[pt++]); // Swap index i and pt in triangles array
		}
		return pt;
	}

	void CalculateNodeAABB(BvhNode& node) {
		const BvhTriangle* tris = triangles();
		node.aabb = AABB(vertices->data()[tris[node.GetLeftIndex()].v0i]);
		for (int i = node.GetLeftIndex(); i < node.GetRightIndex(); i++) {
			node.aabb.Encapsulate(vertices->data()[tris[i].v0i]);
			node.aabb.Encapsulate(vertices->data()[tris[i].v1i]);
			node.aabb.Encapsulate(vertices->data()[tris[i].v2i]);
		}
	}

	// Adapted from Inigo Quilez https://iquilezles.org/articles/
	float ray_tri_intersect(glm::vec3 ro, glm::vec3 rd, const BvhTriangle& tri) const {
		const auto& v0 = vertices->data()[tri.v0i];
		const auto& v1 = vertices->data()[tri.v1i];
		const auto& v2 = vertices->data()[tri.v2i];
		const auto v1v0 = v1 - v0, v2v0 = v2 - v0, rov0 = ro - v0;
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
	float Intersect(const glm::vec3& ro, const glm::vec3& rd, float& depth) const {
		depth = 99999999.9f;
		int minIdx = -1;
		IntersectNode(0, ro, rd, 1.0f / rd, minIdx, depth);
		return minIdx != -1;
	}

	// Recursed func
	void IntersectNode(const int& nodeIndex, const glm::vec3& ro, const glm::vec3& rd, const glm::vec3& inv_rd, int& minTriIdx, float& minDist) const {

		const auto& node = stack[nodeIndex];

		if (node.IsLeaf()) {
			for (int i = node.GetLeftIndex(); i < node.GetRightIndex(); i++) {
				const auto& tri = triangles()[i];
				auto res = ray_tri_intersect(ro, rd, tri);
				if (res > 0.0f && res < minDist) {
					minDist = res;
					minTriIdx = i;
				}
			}
			return;
		}

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

