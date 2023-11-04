#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "Common.h"

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
		int TriangleCount() { return valR - valL; }
	};

	// Generates a new BVH from given points
	void Generate(const std::vector<glm::vec3>& srcPoints);

	bool Exists() const { return stack.size() != 0; }

	float GetClosest(const glm::vec3& pos, uint32_t& minIndex, float& depth) const;

	void GetRange(const glm::vec3& pos, const float& range, std::vector<uint32_t>& buffer) const;

	// Array of bvh nodes, 0 is always root
	std::vector<BvhNode> stack;

private:

	// Single datapoint in bvh
	struct BvhPointData {
		glm::vec3 point;
		uint32_t originalIndex;
	};

	/// Sorted points with indices to original positions
	std::vector<BvhPointData> points;

	/// Number of points after which we stop splitting nodes
	static const int maxNodeEntries = 4;

	// Splits bvh node into 2
	void SplitNode(const int& nodeIdx);

	// Partitions data to 2 sides based on given pos and axis. Right index is exclusive.
	int Partition(const int& low, const int& high, const glm::vec3& splitPos, const int& axis);

	AABB CalculateAABB(const uint32_t& left, const uint32_t& right) const;

	void CalculateNodeAABB(BvhNode& node);

	void IntersectNode(const int& nodeIndex, const glm::vec3& pos, uint32_t& minTriIdx, float& minDist) const;

	void GatherNode(const int& nodeIndex, const float& range, const glm::vec3& pos, std::vector<uint32_t>& buffer) const;
};
