#include "Bvh.h"

#include <thread>
#include <ppl.h> // Parallel for

#include "Engine/Utils.h"

#define MULTI_THREADED_GEN 1

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
	root.aabb = CalculateAABB(root.GetLeftIndex(), root.GetRightIndex());

#if MULTI_THREADED_GEN
	cc_stack.push_back(root);

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
					if (cc_stack[nl].TriangleCount() < 128) SplitNodeRecurse(nl);
					else queue.push(nl);
				}
				if (nr != -1) {
					if (cc_stack[nr].TriangleCount() < 128) SplitNodeRecurse(nr);
					else queue.push(nr);
				}

				cnt--;
			}

			// Cosmic level chance that a thread escapes but the algorithm works fine even with 1T
			if (queue.empty() && cnt == 0) break;
		}
	});

	// Copy to a standard vector to speed up querying
	stack.assign(cc_stack.begin(), cc_stack.end());
#else
	stack.push_back(root);
	SplitNodeRecurse(0); // Single threaded
#endif
}

void Bvh::SplitNodeSingle(const int& nodeIdx, int& nextLeft, int& nextRight) {

#if MULTI_THREADED_GEN
	auto& node = cc_stack[nodeIdx];
#else
	auto& node = stack[nodeIdx];
#endif
	const auto aabbSize = node.aabb.Size();

#if true // SAH Splits

	int minAxis = 0;
	auto minSplitPos = node.aabb.Center();
	float minCost = std::numeric_limits<float>::max();
	const float invNodeArea = 1.0f / node.aabb.AreaHeuristic();

	// Naive = 22.5ms, 6 = 19.7ms, 10 = 19.6ms, 20 = 19.4ms, 100 = 19.0ms
	const float stepsize = 1.0f / 30.0f;
	//const float stepsize = 0.1f / glm::max(aabbSize.x, glm::max(aabbSize.y, aabbSize.z));

	for (uint32_t axis = 0; axis < 3; axis++) {
		
		//const float stepsize = 1.0f / glm::clamp(0.1f / (aabbSize[axis] + 0.000001f), 0.001f, 0.5f);
		
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

#if MULTI_THREADED_GEN
	const int leftStackIndex = (int)(cc_stack.push_back(BvhNode()) - cc_stack.begin());
	const int rightStackIndex = (int)(cc_stack.push_back(BvhNode()) - cc_stack.begin());
#else
	const auto leftStackIndex = (int)stack.size();
	const auto rightStackIndex = (int)stack.size() + 1;
#endif

	left.SetLeftIndex(node.GetLeftIndex());
	left.SetRightIndex(splitPoint);

	right.SetLeftIndex(splitPoint);
	right.SetRightIndex(node.GetRightIndex());

	node.SetLeftChild(leftStackIndex);
	node.SetRightChild(rightStackIndex);

	left.aabb = CalculateAABB(left.GetLeftIndex(), right.GetRightIndex());
	right.aabb = CalculateAABB(right.GetLeftIndex(), right.GetRightIndex());

#if MULTI_THREADED_GEN
	cc_stack[leftStackIndex] = left;
	cc_stack[rightStackIndex] = right;
#else
	stack.push_back(left);
	stack.push_back(right);
#endif

	if (left.TriangleCount() > maxNodeEntries) nextLeft = leftStackIndex;
	if (right.TriangleCount() > maxNodeEntries) nextRight = rightStackIndex;
}

void Bvh::SplitNodeRecurse(const int& nodeIdx) {
	int nextLeft = -1, nextRight = -1;
	SplitNodeSingle(nodeIdx, nextLeft, nextRight);
	if (nextLeft != -1) SplitNodeRecurse(nextLeft);
	if (nextRight != -1) SplitNodeRecurse(nextRight);
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
void Bvh::IntersectNode(const int nodeIndex, const Ray& ray, glm::vec3& normal, int& minTriIdx, float& minDist) const {

	const auto& node = stack[nodeIndex];

	// Leaf -> Check tris
	if (node.IsLeaf()) {
		for (int i = node.GetLeftIndex(); i < node.GetRightIndex(); i++) {

			const BvhTriangle& tri = triangles[i];

			if (tri.originalIndex == ray.mask) continue;

			const float& res = ray_tri_intersect(ray.ro, ray.rd, tri);

			if (res > 0.0f && res < minDist) {
				minDist = res;
				minTriIdx = tri.originalIndex;
				normal = tri.normal;
			}
		}
		return;
	}

	// Node -> Check left/right node
	int nodeA = node.GetLeftChild();
	int nodeB = node.GetRightChild();
	float intersectA = stack[nodeA].aabb.Intersect(ray);
	float intersectB = stack[nodeB].aabb.Intersect(ray);

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

// Calculates whether pos could receive reflected light by the point light at lightpos
bool Bvh::ReflectiveBarycentric(const glm::vec3& lightpos, const glm::vec3& pos, const BvhTriangle& tri, glm::vec3& bary) const {
	using namespace glm;

	// @TODO: This is a self-devised function with 15 dot products... is this the best?

	vec3 v0r = (tri.v0 - lightpos), v1r = (tri.v1 - lightpos), v2r = (tri.v2 - lightpos);
	
	if (dot(v0r, tri.normal) > 0.0f) return false;

	v0r = reflect(v0r, tri.normal);
	v0r = (v0r / dot(v0r, tri.normal)) * dot(pos - tri.v0, tri.normal);
	
	v1r = reflect(v1r, tri.normal);
	v1r = (v1r / dot(v1r, tri.normal)) * dot(pos - tri.v1, tri.normal);
	
	v2r = reflect(v2r, tri.normal);
	v2r = (v2r / dot(v2r, tri.normal)) * dot(pos - tri.v2, tri.normal);

	v0r = tri.v0 + v0r; v1r = tri.v1 + v1r; v2r = tri.v2 + v2r;

	// Barycentric is the point on triangle that reflects here, can return if useful
	bary = Utils::Barycentric(pos, v0r, v1r, v2r);

	if (bary.x > 0.0f && bary.y > 0.0f && bary.z > 0.0f)
		return true; // Pos is inside an area reflected by the light via tri
	return false;
}

bool Bvh::GetClosestReflectiveTri(const glm::vec3& pos, const glm::vec3& lightpos, const float distLimSqr, 
	BvhTriangle& result, glm::vec3& reflectPt) const {
	float minDist = distLimSqr;
	TraverseNode(0, pos, lightpos, distLimSqr, minDist, result, reflectPt);
	return minDist != distLimSqr;
}

void Bvh::TraverseNode(const int& nodeIndex, const glm::vec3& pos, const glm::vec3& lightpos, const float& distLimSqr, 
	float& minDist, Bvh::BvhTriangle& result, glm::vec3& reflectPt) const {

	const auto& node = stack[nodeIndex];

	// Leaf, check tris
	if (node.IsLeaf()) {
		for (int i = node.GetLeftIndex(); i < node.GetRightIndex(); i++) {

			const auto& tri = triangles[i];

			glm::vec3 b;
			if (!ReflectiveBarycentric(lightpos, pos, tri, b)) continue;

			// This tri does reflect here
			glm::vec3 reflPt = tri.v0 * b.x + tri.v1 * b.y + tri.v2 * b.z;
			float dist = Utils::SqrLength(reflPt - pos);
			if (dist < minDist) {
				// We're in reflection range
				// @TODO: 90 deg angle test?
				minDist = dist;
				result = tri;
				reflectPt = reflPt;
			}
		}
		return;
	}

	// Node -> Check left/right

	// bvh.GetClosestReflectiveTri(hitpt, lightpos, lightrange, distlim);
	
	// Query triangle bvh for this
	// Use closest pt on AABB as distance metric
	
	// Prune options by filtering based on dist (light - reflpt) + (pt - reflpt)
	// Prune oblique angles with dot(pt_nrm, refl_trinrm) (90+ degs angle = bad)
	// Have to check all tris within potential refl range and pick closest
	// Tri Reflection func gives barycentric for free when testing for reflection
	
	int nodeA = node.GetLeftChild();
	int nodeB = node.GetRightChild();
	auto distA = Utils::SqrLength(stack[nodeA].aabb.ClosestPoint(pos) - pos);
	auto distB = Utils::SqrLength(stack[nodeB].aabb.ClosestPoint(pos) - pos);

	if (distA < distB) {
		std::swap(nodeA, nodeB);
		std::swap(distA, distB);
	}

	if (distA < distLimSqr && distA < minDist)
		TraverseNode(nodeA, pos, lightpos, distLimSqr, minDist, result, reflectPt);
	if (distB < distLimSqr && distB < minDist)
		TraverseNode(nodeB, pos, lightpos, distLimSqr, minDist, result, reflectPt);
}