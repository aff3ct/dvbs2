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

	this->head                 .push_back(0);
	this->tail                 .push_back(0);
	this->buffer               .push_back(std::vector<int8_t>(this->n_frames * sizeof(T) * 10000));
	this->datatypes            .push_back(typeid(T));
	this->stream_flags         .push_back(ff);
	this->precisions           .push_back(precision);
	this->cols_groups[0].second.push_back(std::make_pair(name, unit));
	this->name_to_col[name] = this->buffer.size() -1;

	return new module::Probe<T>(1, name, *this, this->n_frames);
}

template <typename T>
module::Probe<T>* Reporter_probe
::create_probe(const std::string &name, const std::string &unit, const size_t precision)
{
	return this->create_probe<T>(name, unit, std::ios_base::scientific, precision);
}

template <typename T>
void Reporter_probe
::push(const int col, const T &elt)
{
	std::unique_lock<std::mutex> lck (this->mtx);
	if ((size_t)this->col_size(col) >= this->buffer[col].size())
		return;

	auto buff = reinterpret_cast<T*>(this->buffer[col].data());
	buff[this->head[col]++] = elt;
	this->head[col] %= buffer[col].size();
}

template <typename T>
T Reporter_probe
::pull(const int col, bool &can_pull)
{
	std::unique_lock<std::mutex> lck (this->mtx);
	if (this->col_size(col) == 0)
	{
		can_pull = false;
		return (T)0;
	}

	auto buff = reinterpret_cast<T*>(this->buffer[col].data());
	T elt = buff[this->tail[col]++];
	this->tail[col] %= buffer[col].size();

	can_pull = true;
	return elt;
}

}
}
