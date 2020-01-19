#include "Query.h"
#include "TextQuery.h"
#include <memory>
#include <set>
#include <algorithm>
#include <iostream>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <regex>
#include <stdio.h>
#include <string.h>

using namespace std;
////////////////////////////////////////////////////////////////////////////////
void split(string const& str, vector<string>& out);

std::shared_ptr<QueryBase> QueryBase::factory(const string& s)
{
	regex _and(".+ AND .+");
	regex _or(".+ OR .+");
	regex _not("NOT .+");
	regex _n(R"(.+ \d+ .+)");
	regex _word(R"(\S+)"); 
	std::vector<std::string> results;
	//split string for words
	split(s,results);

	if (regex_match(s, _and)) {
		return std::shared_ptr<AndQuery>(new AndQuery(results.at(0), results.at(2)));
	}
	if (regex_match(s, _or)) {
		return std::shared_ptr<OrQuery>(new OrQuery(results.at(0), results.at(2)));
	}
	if (regex_match(s, _n)) {
		return std::shared_ptr<NQuery>(new NQuery(results.at(0), results.at(2), std::stoi(results.at(1))));
	}
	if (regex_match(s, _not)) {
		return std::shared_ptr<NotQuery>(new NotQuery(results.at(1)));
	}
	if (regex_match(s, _word)) {
		return std::shared_ptr<WordQuery>(new WordQuery(s));
	}
	throw invalid_argument("Unrecognized search");

	return std::shared_ptr<QueryBase>();
 
}
////////////////////////////////////////////////////////////////////////////////
QueryResult NotQuery::eval(const TextQuery &text) const
{
  QueryResult result = text.query(query_word);
  auto ret_lines = std::make_shared<std::set<line_no>>();
  auto beg = result.begin(), end = result.end();
  auto sz = result.get_file()->size();
  
  for (size_t n = 0; n != sz; ++n)
  {
	if (beg==end || *beg != n)
		ret_lines->insert(n);
	else if (beg != end)
		++beg;
  }
  return QueryResult(rep(), ret_lines, result.get_file());
	
}

QueryResult AndQuery::eval (const TextQuery& text) const
{
  QueryResult left_result = text.query(left_query);
  QueryResult right_result = text.query(right_query);
  
  auto ret_lines = std::make_shared<std::set<line_no>>();
  std::set_intersection(left_result.begin(), left_result.end(),
	  right_result.begin(), right_result.end(), 
	  std::inserter(*ret_lines, ret_lines->begin()));

  return QueryResult(rep(), ret_lines, left_result.get_file());
}

QueryResult OrQuery::eval(const TextQuery &text) const
{
  QueryResult left_result = text.query(left_query);
  QueryResult right_result = text.query(right_query);
  
  auto ret_lines = 
	  std::make_shared<std::set<line_no>>(left_result.begin(), left_result.end());

  ret_lines->insert(right_result.begin(), right_result.end());

  return QueryResult(rep(), ret_lines, left_result.get_file());
}
/////////////////////////////////////////////////////////
QueryResult NQuery::eval(const TextQuery &text) const{

	//all lines holding both words
	QueryResult QR = AndQuery::eval(text);

	// for return result lines
	auto ret_lines = std::make_shared<std::set<line_no>>();

	//go over all lines from QR
	for (auto itr = QR.begin(); itr != QR.end(); ++itr) {
		string line = QR.get_file()->at(*itr);

		//get both words index i1 will be first word, i2 second word
		auto i1 = line.find(left_query);
		auto i2 = line.find(right_query);

		if (i1 > i2) {
			swap(i1, i2);
		}

		//for counting words between left-right: spaces will be higher by 1 over words("A B C" = 2 spaces, 1 word)
		auto startPos = line.begin();
		auto _count = count(startPos + i1, startPos + i2, ' ') - 1;

		//insert line if condition met
		if (_count <= dist) {
			ret_lines->insert(*itr);
		}
	}

	return QueryResult(rep(), ret_lines, QR.get_file());
}
/////////////////////////////////////////////////////////

/* split string to tokens with delimiter */
void split(string const& str, vector<string>& out)
{
	// construct a stream from the string
	istringstream stream(str);
	string word;

	while (stream >> word) {
		out.push_back(word);
	}
}