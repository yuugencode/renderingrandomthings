#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <concurrent_vector.h>
#include "concurrent_queue.h"

#include "Engine/Common.h"

// Acceleration structure for 3D points
class BvhPoint {
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
		int ElementCount() { return valR - valL; }
	};

	// Single datapoint in bvh
	struct BvhPointData {
		glm::vec3 point;
		int mask;
	};

	// Generates a new BVH from given points
	void Generate(const std::vector<glm::vec4>& srcPoints);

	bool Exists() const { return stack.size() != 0; }

	// Clears the data this bvh holds
	void Clear();

	// Returns the closest point in this bvh
	void GetClosest(const glm::vec3& pos, float& dist, BvhPointData& d0) const;

	// Returns the 3 closest points in this bvh
	void Get3Closest(const glm::vec3& queryPos, glm::vec3& dists, BvhPointData& d0, BvhPointData& d1, BvhPointData& d2) const;

	// Returns the 4 closest points in this bvh
	void Get4Closest(const glm::vec3& queryPos, glm::vec4& dists, BvhPointData& d0, BvhPointData& d1, BvhPointData& d2, BvhPointData& d3) const;

	// Array of bvh nodes, 0 is root
	std::vector<BvhNode> stack;

private:

	// Concurrent stack used during generation
	concurrency::concurrent_vector<BvhNode> cc_stack;

	// Queue used during generation
	concurrency::concurrent_queue<int> queue;

	/// Sorted points with indices to original positions
	std::vector<BvhPointData> points;

	/// Number of points after which we stop splitting nodes
	static const int maxNodeEntries = 32;

	// Splits bvh node into 2
	void SplitNodeSingle(const int& nodeIdx, int& nextLeft, int& nextRight);
	void SplitNodeRecurse(const int& nodeIdx);

	// Partitions data to 2 sides based on given pos and axis. Right index is exclusive.
	int Partition(const int& low, const int& high, const glm::vec3& splitPos, const int& axis);

	AABB CalculateAABB(const int& left, const int& right) const;

	void CalculateNodeAABB(BvhNode& node);

	void IntersectNode(const int& nodeIndex, const glm::vec3& pos, float& minDist, BvhPointData& d0) const;

	void Gather3Closest(const int& nodeIndex, const glm::vec3& pos, glm::vec3& dists, BvhPointData& d0, BvhPointData& d1, BvhPointData& d2) const;

	void Gather4Closest(const int& nodeIndex, const glm::vec3& pos, glm::vec4& dists, 
		BvhPointData& d0, BvhPointData& d1, BvhPointData& d2, BvhPointData& d3) const;
};
