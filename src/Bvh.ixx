module;

#include <glm/glm.hpp>

export module Bvh;

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
	
	// Abstraction for a single triangle
	struct BvhTriangle { 
		glm::vec3 v0, v1, v2, normal;
		uint32_t originalIndex; // These are sorted around so need to save this, alternatively could have 1 added indirection
		glm::vec3 Centroid() const { return (v0 + v1 + v2) * 0.3333333333f; }
		glm::vec3 Min() const { return glm::min(glm::min(v0, v1), v2); }
		glm::vec3 Max() const { return glm::max(glm::max(v0, v1), v2); }
	};

	std::vector<BvhNode> stack;
	std::vector<BvhTriangle> triangles;

	bool Exists() const { return stack.size() != 0; }

	static const int maxNodeEntries = 3;

	/// <summary> Generates a new BVH from given vertices/tris </summary>
	void Generate(const std::vector<glm::vec3>& srcVertices, const std::vector<uint32_t>& srcTriangles) {
		
		if (srcVertices.size() == 0) return;

		stack.clear();
		triangles.clear();

		triangles.reserve(srcTriangles.size());

		// Remove indirection at the cost of an additional buffer
		for (size_t i = 0; i < srcTriangles.size(); i += 3) {

			const auto& v0 = srcVertices.data()[srcTriangles.data()[i]];
			const auto& v1 = srcVertices.data()[srcTriangles.data()[i + 1]];
			const auto& v2 = srcVertices.data()[srcTriangles.data()[i + 2]];

			triangles.push_back(BvhTriangle {
				.v0 = v0, .v1 = v1, .v2 = v2,
				.normal = glm::normalize(glm::cross(v0 - v1, v0 - v2)),
				.originalIndex = (uint32_t)i,
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

		// @IDEA: For the first ray traversal it might be possible to optimize building using camera information
		// for example partition using dot(viewdir, nrm) + dot(viewdir, ro - tripos) + (pos.x/y/z on largest_axis)
		// this should split backfacing triangles out very quick and prefer tris that are pointing at the cam
		// indirect bounces would have to use a standard bvh
		
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
			if (triangles[i].Max()[axis] < splitPos[axis])
				std::swap(triangles[i], triangles[pt++]); // Swap index i and pt in triangles array
		return pt;
	}

	AABB CalculateAABB(const uint32_t& left, const uint32_t& right) const {
		AABB ret = AABB(triangles[left].v0);
		for (uint32_t i = left; i < right; i++) {
			ret.Encapsulate(triangles[i].v0);
			ret.Encapsulate(triangles[i].v1);
			ret.Encapsulate(triangles[i].v2);
		}
		return ret;
	}

	void CalculateNodeAABB(BvhNode& node) {
		node.aabb = AABB(triangles[node.GetLeftIndex()].v0);
		for (int i = node.GetLeftIndex(); i < node.GetRightIndex(); i++) {
			node.aabb.Encapsulate(triangles[i].v0);
			node.aabb.Encapsulate(triangles[i].v1);
			node.aabb.Encapsulate(triangles[i].v2);
		}
	}

	// Adapted from Inigo Quilez https://iquilezles.org/articles/
	float ray_tri_intersect(const glm::vec3& ro, const glm::vec3& rd, const BvhTriangle& tri) const {
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

	/// <summary> Intersects a ray against this bvh </summary>
	float Intersect(const Ray& ray, glm::vec3& normal, uint32_t& minIndex, float& depth) const {
		depth = 99999999.9f;
		minIndex = -1; // Overflows to max val
		IntersectNode(0, ray, normal, minIndex, depth);
		return minIndex != -1;
	}

	// Recursed func
	void IntersectNode(const int& nodeIndex, const Ray& ray, glm::vec3& normal, uint32_t& minTriIdx, float& minDist) const {

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

			const auto left = node.GetLeftChild();
			const auto right = node.GetRightChild();
			const auto intersectA = stack[left].aabb.Intersect(ray);
			const auto intersectB = stack[right].aabb.Intersect(ray);

			// Both either behind or one is inside -> check the one we're inside of
			if (intersectA < 0.0f && intersectB < 0.0f) {
				if (stack[left].aabb.Contains(ray.ro))
					IntersectNode(left, ray, normal, minTriIdx, minDist);
				if (stack[right].aabb.Contains(ray.ro))
					IntersectNode(right, ray, normal, minTriIdx, minDist);
			}
			// Check if A inside and B outside or miss
			else if (intersectA < 0.0f) {
				if (stack[left].aabb.Contains(ray.ro))
					IntersectNode(left, ray, normal, minTriIdx, minDist);
				if (intersectB != 0.0f && intersectB < minDist)
					IntersectNode(right, ray, normal, minTriIdx, minDist);
			}
			// Check if B inside and A outside or miss
			else if (intersectB < 0.0f) {
				if (stack[right].aabb.Contains(ray.ro))
					IntersectNode(right, ray, normal, minTriIdx, minDist);
				if (intersectA != 0.0f && intersectA < minDist)
					IntersectNode(left, ray, normal, minTriIdx, minDist);
			}
			// Check if both hit and check closer first
			else if (intersectA != 0.0f && intersectB != 0.0f) {
				if (intersectA < intersectB) {
					if (intersectA < minDist) IntersectNode(left, ray, normal, minTriIdx, minDist);
					if (intersectB < minDist) IntersectNode(right, ray, normal, minTriIdx, minDist);
				}
				else {
					if (intersectB < minDist) IntersectNode(right, ray, normal, minTriIdx, minDist);
					if (intersectA < minDist) IntersectNode(left, ray, normal, minTriIdx, minDist);
				}
			}
			// Check if either hit
			else if (intersectA != 0.0f && intersectA < minDist)
				IntersectNode(left, ray, normal, minTriIdx, minDist);
			else if (intersectB != 0.0f && intersectB < minDist)
				IntersectNode(right, ray, normal, minTriIdx, minDist);
		}
	}
};

