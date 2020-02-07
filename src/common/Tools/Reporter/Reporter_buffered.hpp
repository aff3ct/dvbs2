/*!
 * \file
 * \brief Class tools::Reporter_buffered.
 */
#ifndef REPORTER_BUFFERED_HPP_
#define REPORTER_BUFFERED_HPP_

#include "aff3ct.hpp"

namespace aff3ct
{
namespace tools
{

template <typename R = float>
class Reporter_buffered : public Reporter
{
protected:
std::vector<int> head;
std::vector<int> tail;

std::vector<std::vector<R> >buffer;

std::map<std::string, int> column_keys;

public:
	explicit Reporter_buffered(const int nb_cols=1, const int max_size = 100);
	virtual ~Reporter_buffered() = default;

	void push(const R *elt ,        int col);
	void push(const R *elt ,        std::string key);

	void pull(      R *elts, int N, int col);
	void pull(      R *elts, int N, std::string key);

	int col_size(int col){return head[col] >= tail[col] ? head[col]-tail[col] : head[col]-tail[col]+this->buffer[col].size();};
	int full_row_nbr(       );
	void print_buffer();

	virtual report_t report(bool final = false) = 0;

};
}
}

#endif /* REPORTER_BUFFERED_HPP_ */
