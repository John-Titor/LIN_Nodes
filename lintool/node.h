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

#include <list>

#include "param.h"
#include "firmware.h"
#include "exceptions.h"

class Node
{
public:
    typedef std::list<Node *>   List;
    static const unsigned       kNoNode = ~0U;

    Node(unsigned address);
    ~Node();

    EXCEPTION(Exception, ExScanFailed);
    EXCEPTION(Exception, ExUpdateFailed);
    EXCEPTION(Exception, ExDefaultFailed);

    static void     scan(unsigned address = kNoNode);
    static bool     exists(unsigned address);

    static void     clear() { _list.clear(); }
    static List     &nodes() { return _list; }

    static Node     *matching(unsigned address, unsigned function);

    unsigned        address() const { return _address; }
    unsigned        function() { return params().function(); }
    ParamSet        &params() { return _params; }

    void            update(bool verify, bool save_params);
    void            set_defaults();

private:
    static List     _list;

    ParamSet        _params;
    unsigned        _address;

    void            enter_bootloader();
    void            leave_bootloader(bool check);
    static void     _scan(unsigned address);
};

