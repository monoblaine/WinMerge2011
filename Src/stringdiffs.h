/**
 * @file  stringdiffs.h
 *
 * @brief Interface file declaring sd_ComputeWordDiffs (q.v.)
 *
 */
#pragma once

/** @brief One difference between two strings */
struct wdiff
{
	int start[2]; // 0-based, eg, start[0] is from str1
	int end[2]; // 0-based, eg, end[1] is from str2
	wdiff(int s1 = 0, int e1 = 0, int s2 = 0, int e2 = 0)
	{
		start[0] = s1;
		start[1] = s2;
		end[0] = e1;
		end[1] = e2;
	}
	bool IsInsert() const
	{
		return start[0] == end[0] || start[1] == end[1];
	}
	// Predicates for use with find_if()
	class next_to
	{
		int const ind;
		int const pos;
	public:
		next_to(int ind, int pos): ind(ind), pos(pos) { }
		bool operator()(wdiff const &wd) const { return wd.end[ind] > pos; }
	};
	class prev_to
	{
		int const ind;
		int const pos;
	public:
		prev_to(int ind, int pos): ind(ind), pos(pos) { }
		bool operator()(wdiff const &wd) const { return wd.start[ind] < pos; }
	};
};

void sd_SetBreakChars(LPCTSTR breakChars);

void sd_ComputeWordDiffs(LPCTSTR str1, int len1, LPCTSTR str2, int len2,
	bool case_sensitive, int whitespace, int breakType, bool byte_level,
	std::vector<wdiff> &diffs);
