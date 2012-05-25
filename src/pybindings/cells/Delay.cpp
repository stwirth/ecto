/*
 * Copyright (c) 2012, Stephan Wirth <stephan.wirth@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <ecto/ecto.hpp>
#include <queue>

namespace ecto
{
  namespace bp = boost::python;

  struct Delay
  {
    static void
    declare_params(tendrils& parms)
    {
      parms.declare<int>("num", "Number of iterations to delay the output.");
    }
    static void
    declare_io(const tendrils& parms, tendrils& in, tendrils& out)
    {
      in.declare<tendril::none>("in", "Any type");
      out.declare<tendril::none>("out", "Any type");
    }
    void configure(const tendrils& p, const tendrils& in, const tendrils& out)
    {
      num_ = p["num"];
    }
    int
    process(const tendrils& in, const tendrils& out)
    {
      queue_.push(*(in["in"]));
      if (queue_.size() > *num_)
      {
        out["out"] << queue_.front();
        queue_.pop();
        return ecto::OK;
      }
      else
      {
        return ecto::BREAK;
      }
    }

    spore<int> num_;
    std::queue<tendril> queue_;

  };
}

ECTO_CELL(cells, ecto::Delay, "Delay", "Delays the output of an object.");
