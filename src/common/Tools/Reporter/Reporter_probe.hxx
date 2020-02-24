#include "Module/Probe/Probe.hpp"
#include "Reporter_probe.hpp"

namespace aff3ct
{
namespace tools
{

template <typename T>
module::Probe<T>* Reporter_probe
::create_probe(const std::string &name, const std::string &unit, const std::ios_base::fmtflags ff, const size_t precision)
{
	if (name_to_col.count(name))
	{
		std::stringstream message;
		message << "'name' already exist in this reporter ('name' = " << name << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	if (this->buffer.size() > 100)
	{
		std::stringstream message;
		message << "'buffer.size()' has to be smaller than 'mtx.size()' ('buffer.size()' = " << this->buffer.size()
		        << ", 'mtx.size()' = " << this->mtx.size() << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	this->head                 .push_back(0);
	this->tail                 .push_back(0);
	this->buffer               .push_back(std::vector<int8_t>(this->n_frames * sizeof(T) * 100));
	this->datatypes            .push_back(typeid(T));
	this->stream_flags         .push_back(ff);
	this->precisions           .push_back(precision);
	this->cols_groups[0].second.push_back(std::make_pair(name, unit));
	this->name_to_col[name] = this->buffer.size() -1;
	this->col_to_name[this->buffer.size() -1] = name;

	return new module::Probe<T>(1, name, *this, this->n_frames);
}

template <typename T>
module::Probe<T>* Reporter_probe
::create_probe(const std::string &name, const std::string &unit, const size_t precision)
{
	return this->create_probe<T>(name, unit, std::ios_base::scientific, precision);
}

template <typename T>
size_t Reporter_probe
::col_size(const int col, const size_t buffer_size)
{
	return head[col] >= tail[col] ? head[col]-tail[col] : head[col]-tail[col] + buffer_size;
}

template <typename T>
bool Reporter_probe
::push(const int col, const T &elt)
{
	std::unique_lock<std::mutex> lck(this->mtx[col]);
	const auto buff_size = this->buffer[col].size() / sizeof(T);
	if (this->col_size<T>(col, buff_size) >= buff_size)
		return false;

	auto buff = reinterpret_cast<T*>(this->buffer[col].data());
	buff[this->head[col] == buff_size ? 0 : this->head[col]] = elt;
	this->head[col] = this->head[col] == buff_size ? 1 : this->head[col] +1;

	return true;
}

template <typename T>
T Reporter_probe
::pull(const int col, bool &can_pull)
{
	std::unique_lock<std::mutex> lck(this->mtx[col]);
	const auto buff_size = this->buffer[col].size() / sizeof(T);
	if (this->col_size<T>(col, buff_size) == 0)
	{
		can_pull = false;
		return (T)0;
	}

	auto buff = reinterpret_cast<T*>(this->buffer[col].data());
	T elt = buff[this->tail[col] == buff_size ? 0 : this->tail[col]];
	this->tail[col] = this->tail[col] == buff_size ? 1 : this->tail[col] +1;

	can_pull = true;
	return elt;
}

}
}
