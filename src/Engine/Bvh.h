#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <concurrent_vector.h>
#include "concurrent_queue.h"

#include "Engine/Common.h"

// Acceleration structure for triangles
class Bvh {
public:

	struct BvhNode {

		// Negative values = nonleaf, indices are to stack, encoded with -1 offset to avoid 0 index
		// Positive values = leaf, indices are to data, encoded as is
		int valL = 0, valR = 0;
	public:

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
		int TriangleCount() const { return valR - valL; }
	};

	// Abstraction for a single triangle
	struct BvhTriangle {
		glm::vec3 v0, v1, v2, normal;
		int originalIndex; // These are sorted around so need to save this, coulda instead added 1 indirection but slower
		glm::vec3 Centroid() const { return (v0 + v1 + v2) * 0.3333333333f; }
		glm::vec3 Min() const { return glm::min(glm::min(v0, v1), v2); }
		glm::vec3 Max() const { return glm::max(glm::max(v0, v1), v2); }
	};

	// Generates a new BVH from given vertices/tris
	void Generate(const std::vector<glm::vec3>& srcVertices, const std::vector<uint32_t>& srcTriangles);

	bool Exists() const { return stack.size() != 0; }

	// Intersects a ray against this bvh
	float Intersect(const Ray& ray, glm::vec3& normal, int& minIndex, float& depth) const;

	// Array of bvh nodes, 0 is always root
	std::vector<BvhNode> stack;

	/// Sorted triangles with indices to original positions
	std::vector<BvhTriangle> triangles;

	// Returns the closest triangle that potentially might reflect light to pos cast by a pointlight in lightpos
	bool GetClosestReflectiveTri(const glm::vec3& pos, const glm::vec3& lightpos, const float& distLimSqr, const int& triMask,
		BvhTriangle& result, glm::vec3& reflectPt) const;

private:

	// Concurrent stack used during generation
	concurrency::concurrent_vector<BvhNode> cc_stack;

	// Queue used during generation
	concurrency::concurrent_queue<int> queue;

	/// Number of tris after which we stop splitting nodes
	static const int maxNodeEntries = 4;

	// Splits bvh node into 2
	void SplitNodeSingle(const int& nodeIdx, int& nextLeft, int& nextRight);

	void SplitNodeRecurse(const int& nodeIdx);

	// Partitions data to 2 sides based on given pos and axis. Right index is exclusive.
	int Partition(const int& low, const int& high, const glm::vec3& splitPos, const int& axis);

	AABB CalculateAABB(const int& left, const int& right) const;

	float ray_tri_intersect(const glm::vec3& ro, const glm::vec3& rd, const BvhTriangle& tri) const;

	void IntersectNode(const int nodeIndex, const Ray& ray, glm::vec3& normal, int& minTriIdx, float& minDist) const;

	void TraverseNode(const int& nodeIndex, const glm::vec3& pos, const glm::vec3& lightpos, const int& triMask,
		float& minDist, Bvh::BvhTriangle& result, glm::vec3& reflectPt) const;

	bool ReflectiveBarycentric(const glm::vec3& lightpos, const glm::vec3& pos, const BvhTriangle& tri, glm::vec3& bary) const;
};
