/*
 * Copyright (c) 2012-2015, Mike Smith, <msmith@purgatory.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * o Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include <stdint.h>

#include <list>
#include <cstdio>
#include <sstream>

#include <lin_defs.h>

#include "json.h"
#include "exceptions.h"

class Param
{
public:
    typedef std::list<Param *>  List;
    static const unsigned   kMaxAddress = 0xffff;

    Param(unsigned address, unsigned function = board_function::kUnknown) :
        _address(address),
        _function(function)
    {}

    EXCEPTION(Exception, ExBadValue);
    EXCEPTION(Exception, ExNotSettable);
    EXCEPTION(Exception, ExNotValid);
    EXCEPTION(Exception, ExNonexistent);

    /// @return             A formatted description of the parameter and
    ///                     the local copy of the value. Must be freed by
    ///                     the caller.
    char                *format() const;

    /// Synchronise the current value with the node
    ///
    /// @param force        Forces parameter to be written even if it's not dirty
    ///
    void                sync(bool force = false);

    /// Update the local copy of the value
    ///
    /// @param value        New value to set.
    ///
    void                set(unsigned value);

    /// @return             The local copy of the value.
    unsigned            get();

    /// @return             The default for the value (meaningless unless is_settable
    ///                     returns true).
    unsigned            get_default();

    /// @return             True if this parameter exists.
    bool                exists() const;

    /// @return             The address of this parameter
    unsigned            address() const { return _address; }

    /// @return             The encoding applicable to this parameter.
    unsigned            encoding() const;

    /// @return             The name of this parameter.
    const char          *name() const;

    /// @return             The info string applicable to the local copy
    ///                     of the value.
    const char          *info() const;

    /// @return             True if the local copy of the value is valid.
    bool                is_valid() const { return _valid; }

    /// @return             True if the local copy of the value has been
    ///                     writen and not yet synced to the node.
    bool                is_dirty() const { return _dirty; }

    /// @return             True if the parameter can be set.
    bool                is_settable() const 
    {
        return (_address >= Generic::kParamConfigBase) && (_address < Generic::kParamConfigTop);
    }

private:
    bool                _valid = false;
    bool                _dirty = false;
    unsigned            _value = 0;

    unsigned            _address;
    unsigned            _function;

    void                fetch();
    void                store();
};

class ParamSet
{
public:
    typedef std::list<ParamSet *>   List;

    ParamSet(unsigned node);
    ~ParamSet();

    /// @return             A formatted description of the node that this
    ///                     parameter set applies to.
    std::ostringstream  &identity();

    /// Synchronises all parameters in the set with the node.
    ///
    /// @param force        Forces parameters to be written even if they aren't dirty.
    ///
    void                sync(bool force = false);

    /// @return             True if any parameter in the set has been written
    ///                     and not yet synced to the node.
    bool                is_dirty() const;

    /// Locate a parameter for a specific parameter address.
    ///
    /// @param address      The parameter address being sought.
    /// @return             The parameter.
    ///
    Param               *find(unsigned address) const;

    /// Locate a parameter by name
    ///
    /// @param name         The name of the parameter being sought.
    /// @return             The parameter.
    ///
    Param               *param_for_name(const char *name);

    /// Populate the parameter set from a de-serialised JSON representation.
    ///
    /// @param fromNode     The Json object to populate from.
    ///
    void                set(const json::Object &fromNode);

    /// @return             The node address this parameter set applies to.
    unsigned            node() const { return _node; }

    /// @return             The function this parameter set applies to.
    unsigned            function() const { return _function; }

    /// @return             A list of all parameter sets currently loaded.
    Param::List         &list() { return _params; }

private:
    unsigned            _node;
    unsigned            _function = board_function::kUnknown;
    Param::List         _params;
    std::ostringstream  _identity;
};

class ParamDB
{
public:
    ParamDB();
    ~ParamDB();

    EXCEPTION(Exception, ExDBInvalid);

    /// Populate the DB from a file
    ///
    /// @param              The file to read from.
    ///
    void                read(const char *path);

    /// Write the DB to a file
    ///
    /// @param              The file to write.
    ///
    void                write(const char *path);

    /// Store a parameter set into the DB.
    ///
    /// @param pset         The parameter set to store.
    ///
    void                store(ParamSet &pset);

    /// Fetch a parameter set from the DB.
    ///
    /// @param pset         The parameter set to fetch.
    void                fetch(ParamSet &pset);

    /// @return             The collection of nodes in the database
    json::Array         &nodes() { return _db; }

private:
    json::Array         _db;
};
