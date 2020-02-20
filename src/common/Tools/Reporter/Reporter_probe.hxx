#include "Module/Probe/Probe.hpp"
#include "Reporter_probe.hpp"

namespace aff3ct
{
namespace tools
{

template <typename T>
module::Probe<T>* Reporter_probe
::create_probe(const std::string &name, const std::string &unit)
{
	if (column_keys.count(name))
	{
		std::stringstream message;
		message << "'name' already exist in this reporter ('name' = " << name << ").";
		throw tools::invalid_argument(__FILE__, __LINE__, __func__, message.str());
	}

	this->head                 .push_back(0);
	this->tail                 .push_back(0);
	this->buffer               .push_back(std::vector<int8_t>(this->n_frames * sizeof(T) * 10000));
	this->types                .push_back(typeid(T));
	this->cols_groups[0].second.push_back(std::make_pair(name, unit));
	this->column_keys[name] = this->buffer.size() -1;

	return new module::Probe<T>(1, name, *this, this->n_frames);
}

template <typename T>
void Reporter_probe
::push(const T &elt, const int col)
{
	if ((size_t)this->col_size(col) >= this->buffer[col].size())
		return;

	auto buff = reinterpret_cast<T*>(this->buffer[col].data());
	buff[this->head[col]++] = elt;
	this->head[col] %= buffer[col].size();
}

template <typename T>
void Reporter_probe
::push(const T &elt, const std::string &key)
{
	this->push<T>(elt, this->column_keys[key]);
}

template <typename T>
T Reporter_probe
::pull(const int col, bool &can_pull)
{
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

template <typename T>
T Reporter_probe
::pull(const std::string& key, bool &can_pull)
{
	return this->pull<T>(this->column_keys[key], can_pull);
}

}
}
