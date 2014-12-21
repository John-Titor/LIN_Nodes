/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <msmith@purgatory.org> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <err.h>

#include "param.h"
#include "link.h"

char *
Param::format() const
{
    char *str;

    asprintf(&str, "%u %s %u %s", _address, name(), _value, info());
    return str;
}

void
Param::sync(bool force)
{
    if (!_valid) {
        fetch();

    } else if (_dirty || force) {
        store();
    }
}

void
Param::set(unsigned value)
{
    if (is_settable()) {
        if (Encoding::invalid(encoding(), value)) {
            RAISE(ExBadValue, "invalid value " << value << " for " << name());
        }

        _value = value;
        _dirty = true;
        _valid = true;
        return;
    }

    RAISE(ExNotSettable, "cannot set " << name());
}

unsigned
Param::get()
{
    if (_valid) {
        return _value;
    }

    RAISE(ExNotValid, name() << " is not valid at this time");
}

bool
Param::exists() const
{
    if (Generic::param_exists(_address)) {
        return true;
    }

    switch (_function) {
    case board_function::kMaster:
        return Master::param_exists(_address);

    case board_function::kPowerV3:
        return PowerV3::param_exists(_address);

    case board_function::kUnconfigured:
        return Bootloader::param_exists(_address);

    default:
        break;
    }

    return false;
}

void
Param::fetch()
{
    _dirty = false;
    _valid = true;

    try {
        _value = Link::read_param(_address);

    } catch (Link::ExNoParam) {
        _valid = false;

    } catch (...) {
        _valid = false;
        throw;
    }
}

void
Param::store()
{
    // XXX magic numbers
    if ((_address >= 0x400) && (_address < 0x500)) {
        Link::write_param(_address, _value);
        _dirty = false;
    }
}

unsigned
Param::encoding() const
{
    unsigned value = kEncoding_none;

    switch (_function) {
    case board_function::kMaster:
        value = Master::param_encoding(_address);
        break;

    case board_function::kPowerV3:
        value = PowerV3::param_encoding(_address);
        break;

    case board_function::kUnconfigured:
        value = Bootloader::param_encoding(_address);
        break;
    }

    return value;
}

const char *
Param::name() const
{
    const char *str = nullptr;

    switch (_function) {
    case board_function::kMaster:
        str = Master::param_name(_address);
        break;

    case board_function::kPowerV3:
        str = PowerV3::param_name(_address);
        break;

    case board_function::kUnconfigured:
        str = Bootloader::param_name(_address);
        break;
    }

    if (str == nullptr) {
        str = Generic::param_name(_address);
    }

    return str;
}

const char *
Param::info() const
{
    const char *str = Encoding::info(encoding(), _value);

    if (str == nullptr) {
        str = "";
    }

    return str;
}

////////////////////////////////////////////////////////////////////////////////

ParamSet::ParamSet(unsigned node) :
    _node(node)
{
    Link::set_node(_node);

    _function = Link::read_param(Generic::kParamBoardFunction);

    // XXX magic numbers
    for (auto address = 0; address < 0x10000U; address++) {
        if (Param(address, _function).exists()) {
            auto p = new Param(address, _function);

            _params.push_back(p);
        }
    }
}

ParamSet::~ParamSet()
{
    for (;;) {
        auto p = _params.front();

        if (p == nullptr) {
            break;
        }

        _params.pop_front();
        delete p;
    }
}

char *
ParamSet::identity() const
{
    char *str;

    asprintf(&str, "[%u:%u:%s]",
             _node,
             _function,
             Encoding::info(kEncoding_board_function, _function));
    return str;
}

bool
ParamSet::is_dirty() const
{
    for (auto p : _params) {
        if (p->is_dirty()) {
            return true;
        }
    }

    return false;
}

Param *
ParamSet::find(unsigned address) const
{
    for (auto p : _params) {
        if (p->address() == address) {
            return p;
        }
    }

    return nullptr;
}

void
ParamSet::sync(bool force)
{
    Link::set_node(_node);

    for (auto p : _params) {
        p->sync(force);
    }
}

void
ParamSet::set(const json::Object &fromNode)
{
    // Find the parameter and make sure we want to set it
    const char *name = fromNode["name"].ToString().c_str();
    auto p = param_for_name(name);

    if (p == nullptr) {
        RAISE(Param::ExNonexistent, "parameter " << name << " does not exist in this context");
    }

    if (!p->is_settable()) {
        return;
    }

    // Work out the value we are going to set
    uint16_t value = fromNode["value"].ToInt();
    const char *info = fromNode["info"].ToString().c_str();

    // Do we have a named value?
    if (strcmp(info, "")) {
        // prefer a named value (since encodings may change)
        if (!Encoding::value(p->encoding(), info, value)) {
            RAISE(Param::ExNotValid, "parameter " << name << " cannot be set to '" << info << "' / " << value);
        }
    }

    Link::set_node(_node);
    p->set(value);
}

Param *
ParamSet::param_for_name(const char *name)
{
    for (auto p : _params) {
        if (!strcmp(p->name(), name)) {
            return p;
        }
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////

ParamDB::ParamDB()
{
}

ParamDB::~ParamDB()
{
}

void
ParamDB::read(const char *path)
{
    std::ifstream in(path);
    std::string str;
    in >> str;
    in.close();

    try {
        _db = json::Deserialize(str);

    } catch (const std::runtime_error &e) {
        RAISE(ExDBInvalid, "Json parse error: " << e.what());
    }
}

void
ParamDB::write(const char *path)
{
    std::ofstream out(path);
    out << json::Serialize(_db).c_str();
    out.close();
}

void
ParamDB::store(ParamSet &pset)
{
    auto params = json::Array();

    for (auto p : pset.list()) {

        if (p->is_valid()) {
            auto param = json::Object();
            param["name"]       = p->name();
            param["address"]    = (int)p->address();
            param["value"]      = (int)p->get();
            param["info"]       = p->info();
            params.push_back(param);
        }
    }

    auto node = json::Object();
    node["name"]        = Encoding::info(kEncoding_board_function, pset.function());
    node["node"]        = (int)pset.node();
    node["function"]    = (int)pset.function();
    node["parameters"]  = params;

    _db.push_back(node);
}
