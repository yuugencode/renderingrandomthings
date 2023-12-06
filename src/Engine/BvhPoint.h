#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <concurrent_vector.h>
#include "concurrent_queue.h"

#include "Engine/Common.h"

// Acceleration structure for 3D points
template <typename T>
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
		void SetLeftIndex(int val) { valL = val; }
		void SetRightIndex(int val) { valR = val; }
		void SetLeftChild(int val) { valL = -val - 1; }
		void SetRightChild(int val) { valR = -val - 1; }
		int ElementCount() { return valR - valL; }
	};

	// Single datapoint in bvh
	struct BvhPointData {
		glm::vec3 point;
		T payload;
	};

	// Generates a new BVH from given points
	void Generate(const void* data, int count);

	bool Exists() const { return stack.size() != 0; }

	// Clears the data this bvh holds
	void Clear();

	// Returns the N closest points in this bvh
	template <int N>
	void GetNClosest(const glm::vec3& queryPos, float* dists, BvhPointData* data) const;

	// Array of bvh nodes, 0 is root
	std::vector<BvhNode> stack;

	/// Sorted points with indices to original positions
	std::vector<BvhPointData> points;

private:

	// Concurrent stack used during generation
	concurrency::concurrent_vector<BvhNode> cc_stack;

	// Queue used during generation
	concurrency::concurrent_queue<int> queue;

	/// Number of points after which we stop splitting nodes
	static const int maxNodeEntries = 32;

	// Splits bvh node into 2
	void SplitNodeSingle(int nodeIdx, int& nextLeft, int& nextRight);
	void SplitNodeRecurse(int nodeIdx);

	// Partitions data to 2 sides based on given pos and axis. Right index is exclusive.
	int Partition(int low, int high, const glm::vec3& splitPos, int axis);

	AABB CalculateAABB(int left, int right) const;

	template <int N>
	void GatherNClosest(const int& nodeIndex, const glm::vec3& pos, float* dists, BvhPointData* data) const;
};
