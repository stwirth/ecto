//
// Copyright (c) 2011, Willow Garage, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the Willow Garage, Inc. nor the names of its
//       contributors may be used to endorse or promote products derived from
//       this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#include <ecto/all.hpp>
#include "plasm/impl.hpp"

#include <ecto/tendrils.hpp>
#include <ecto/edge.hpp>
#include <ecto/cell.hpp>
#include <ecto/impl/graph_types.hpp>

#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>

#include <string>
#include <map>
#include <set>
#include <utility>
#include <deque>

#include <ecto/ecto.hpp>
#include <ecto/serialization/registry.hpp>
#include <ecto/serialization/cell.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/preprocessor/stringize.hpp>

namespace ecto
{
  using namespace graph;

  //see http://www.graphviz.org/content/node-shapes for reference.
  const char* table_str = BOOST_PP_STRINGIZE(
      <TABLE BORDER="0" CELLBORDER="1" CELLSPACING="0" CELLPADDING="4">
      %s
      <TR>
      %s
      %s
      </TR>
      %s
      %s
      </TABLE>
  );

  const char* param_str_1st = BOOST_PP_STRINGIZE(
      <TD PORT="p_%s" BGCOLOR="lightblue">%s<BR/><FONT POINT-SIZE="8">%s</FONT></TD>
  );

  const char* param_str_N = BOOST_PP_STRINGIZE(
      <TR>
      <TD PORT="p_%s" BGCOLOR="lightblue">%s<BR/><FONT POINT-SIZE="8">%s</FONT></TD>
      </TR>
  );

  const char* output_str = BOOST_PP_STRINGIZE(
      <TD PORT="o_%s" BGCOLOR="indianred1">%s<BR/><FONT POINT-SIZE="8">%s</FONT></TD>
  );

  namespace {
    // see
    // http://en.wikipedia.org/wiki/HSL_and_HSV#Converting_to_RGB
    // for points on a dark background you want somewhat lightened
    // colors generally... back off the saturation (s)
    static void hsv2rgb(float h, float s, float v, float& r, float& g, float& b)
    {
      float c = v * s;
      float hprime = h/60.0;
      float x = c * (1.0 - fabs(fmodf(hprime, 2.0f) - 1));

      r = g = b = 0;

      if (hprime < 1) {
        r = c; g = x;
      } else if (hprime < 2) {
        r = x; g = c;
      } else if (hprime < 3) {
        g = c; b = x;
      } else if (hprime < 4) {
        g = x; b = c;
      } else if (hprime < 5) {
        r = x; b = c;
      } else if (hprime < 6) {
        r = c; b = x;
      }

      float m = v - c;
      r += m; g+=m; b+=m;
    }
  }

  struct vertex_writer
  {
    graph_t* g;

    vertex_writer(graph_t* g_)
        :
          g(g_)
    {
    }

    std::string htmlescape(const std::string& in)
    {
      const boost::regex esc_lt("[<]");
      const std::string rep_lt("&lt;");
      const boost::regex esc_gt("[>]");
      const std::string rep_gt("&gt;");

      std::string htmlescaped_name = in;
      htmlescaped_name = boost::regex_replace(htmlescaped_name, esc_lt, rep_lt, boost::match_default);
      htmlescaped_name = boost::regex_replace(htmlescaped_name, esc_gt, rep_gt, boost::match_default);
      return htmlescaped_name;
    }

    void
    operator()(std::ostream& out, graph_t::vertex_descriptor vd)
    {


      cell_ptr c = (*g)[vd];
      int n_inputs = c->inputs.size();
      int n_outputs = c->outputs.size();
      int n_params = c->parameters.size();
      std::string htmlescaped_name = htmlescape(c->name());

      std::string inputs;
      const static char* input_str = BOOST_PP_STRINGIZE(<TD PORT="i_%s" BGCOLOR="springgreen">%s<BR/>
                                                        <FONT POINT-SIZE="8">%s</FONT></TD>);
      BOOST_FOREACH(const tendrils::value_type& x, c->inputs)
          {
            std::string key = x.first;
            if (inputs.empty())
              inputs = "<TR>\n";
            inputs += boost::str(boost::format(input_str) % key % key % htmlescape(x.second->type_name())) + "\n";
          }
      if (!inputs.empty())
        inputs += "</TR>";

      std::string outputs;
      BOOST_FOREACH(const tendrils::value_type& x, c->outputs)
          {
            std::string key = x.first;
            if (outputs.empty())
              outputs = "<TR>\n";
            outputs += boost::str(boost::format(output_str) % key % key % htmlescape(x.second->type_name())) + "\n";
          }
      if (!outputs.empty())
        outputs += "</TR>";

      float r, g, b; // s, v == 1
      float h = (c->stats.ncalls % 10) * 36.0;

      hsv2rgb(h, c->stats.on ? 1 : 0.5, 1, r, g, b);

      std::string color = str(boost::format("#%|02X|%|02X|%|02X|") % int(r*255) % int(g*255) % int(b*255));

      const static char* cell_str = BOOST_PP_STRINGIZE(<TD ROWSPAN="%d" COLSPAN="%d" BGCOLOR="%s">%s<BR/>
                                                       <FONT POINT-SIZE="8">%s</FONT><BR/>
                                                       tick: %3u
                                                       </TD>);
      std::string cellrow = boost::str(
          boost::format(cell_str)
          % std::max(1,n_params)
          % int(std::max(1,std::max(n_inputs, n_outputs)))
          % color
          % htmlescaped_name
          % htmlescape(c->type())
          % c->stats.ncalls);
      std::string p1, pN;
      BOOST_FOREACH(const tendrils::value_type& x, c->parameters)
          {
            std::string key = x.first;
            if (p1.empty())
              p1 = boost::str(boost::format(param_str_1st) % key % key % htmlescape(x.second->type_name())) + "\n";
            else
              pN += boost::str(boost::format(param_str_N) % key % key % htmlescape(x.second->type_name())) + "\n";
          }

      std::string table = boost::str(boost::format(table_str) % inputs % cellrow % p1 % pN % outputs);
      out << "[label=<" << table << ">]";
    }
  };

  struct edge_writer
  {
    graph_t* g;

    edge_writer(graph_t* g_)
        :
          g(g_)
    {
    }

    void
    operator()(std::ostream& out, graph_t::edge_descriptor ed)
    {
      boost::format fmt("[headport=\"i_%u\" tailport=\"o_%u\" label=\"%u\" penwidth=\"%f\"]\n");
      out << fmt % (*g)[ed]->to_port() % (*g)[ed]->from_port() % (*g)[ed]->size() % ((*g)[ed]->size() + 0.5);
    }
  };

  struct graph_writer
  {
    void
    operator()(std::ostream& out) const
    {
      out << "graph [rankdir=TB, ranksep=1]" << std::endl;
      out << "edge [labelfontsize=8]" << std::endl;
      out << "node [shape=plaintext]" << std::endl;
    }
  };

  plasm::plasm() : impl_(new impl) { }


  plasm::~plasm() { }

  void
  plasm::insert(cell_ptr mod)
  {
    if (movie_out.size() && mod->bsig_process.empty())
      mod->bsig_process.connect(boost::bind(&plasm::frame, this, _1, _2));
    impl_->insert_module(mod);
  }

  void
  plasm::connect(cell_ptr from, const std::string& output, cell_ptr to, const std::string& input)
  {
    if (movie_out.size() && from->bsig_process.empty())
      from->bsig_process.connect(boost::bind(&plasm::frame, this, _1, _2));
    if (movie_out.size() && to->bsig_process.empty())
      to->bsig_process.connect(boost::bind(&plasm::frame, this, _1, _2));
    impl_->connect(from, output, to, input);
  }


  void
  plasm::viz(std::ostream& out) const
  {
    boost::write_graphviz(out, impl_->graph, vertex_writer(&impl_->graph), edge_writer(&impl_->graph), graph_writer());
  }

  std::string
  plasm::viz() const
  {
    std::stringstream ss;
    viz(ss);
    return ss.str();
  }

  void
  plasm::disconnect(cell_ptr from, const std::string& output, cell_ptr to, const std::string& input)
  {
    impl_->disconnect(from, output, to, input);
  }

  graph::graph_t&
  plasm::graph()
  {
    return impl_->graph;
  }

  const graph::graph_t&
  plasm::graph() const
  {
    return impl_->graph;
  }

  std::size_t
  plasm::size() const
  {
    return num_vertices(impl_->graph);
  }

  namespace {
    struct get_first
    {
      template <typename T>
      cell_ptr
      operator()(T& t) const
      {
        return t.first;
      }
    };
  }

  std::vector<cell_ptr>
  plasm::cells() const
  {
    std::vector<cell_ptr> c;
    std::transform(impl_->mv_map.begin(), impl_->mv_map.end(), std::back_inserter(c), get_first());
    return c;
  }

  void plasm::reset_ticks()
  {
    {
      graph_t::vertex_iterator beg, end;
      tie(beg, end) = vertices(impl_->graph);
      while(beg != end)
        {
          cell_ptr c = impl_->graph[*beg];
          c->reset_tick();
          ++beg;
        }
    }

    {
      graph_t::edge_iterator beg, end;
      tie(beg, end) = edges(impl_->graph);
      while(beg != end)
        {
          edge_ptr e = impl_->graph[*beg];
          while(e->size() > 0)
            e->pop_front();
          ++beg;
        }
    }
  }

  void plasm::set_movie_out(const std::string& s)
  {
    movie_out = s;
    movie_frame = 0;
  }

  void plasm::configure_all()
  {
    BOOST_FOREACH(impl::ModuleVertexMap::value_type& x, impl_->mv_map)
    {
      x.first->configure();
    }
  }

  void
  plasm::frame(cell& c, bool onoff)
  {
    boost::mutex::scoped_lock lock(movie_mtx);
    ECTO_LOG_DEBUG("plasm::frame %u %s@%p %u %u", movie_frame % c.name() % &c % onoff % c.stats.ncalls);
    std::string ofname = str(boost::format(movie_out) % movie_frame);
    std::ofstream ofs(ofname.c_str());
    viz(ofs);
    ofs.close();
    ++movie_frame;
  }

  void
  plasm::check() const
  {
    graph_t& g(impl_->graph);
    graph_t::vertex_iterator begin, end;
    tie(begin, end) = boost::vertices(g);
    while (begin != end)
    {
      cell_ptr m = g[*begin];
      std::set<std::string> in_connected, out_connected;

      //verify all required inputs are connected
      graph_t::in_edge_iterator b_in, e_in;
      tie(b_in, e_in) = boost::in_edges(*begin, g);
      while (b_in != e_in)
      {
        edge_ptr in_edge = g[*b_in];
        cell_ptr from_module = g[source(*b_in, g)];
        in_connected.insert(in_edge->to_port());
        ++b_in;
      }

      for (tendrils::const_iterator b_tend = m->inputs.begin(), e_tend = m->inputs.end(); b_tend != e_tend; ++b_tend)
      {
        if (b_tend->second->required() && in_connected.count(b_tend->first) == 0)
        {
          BOOST_THROW_EXCEPTION(except::NotConnected()
                                << except::tendril_key(b_tend->first)
                                << except::cell_name(m->name()));
        }
      }

      //verify the outputs are connected
      graph_t::out_edge_iterator b_out, e_out;
      tie(b_out, e_out) = boost::out_edges(*begin, g);
      while (b_out != e_out)
      {
        edge_ptr out_edge = g[*b_out];
        out_connected.insert(out_edge->from_port());
        ++b_out;
      }

      for (tendrils::const_iterator b_tend = m->outputs.begin(), e_tend = m->outputs.end(); b_tend != e_tend; ++b_tend)
      {
        if (b_tend->second->required() && out_connected.count(b_tend->first) == 0)
        {
          BOOST_THROW_EXCEPTION(except::NotConnected()
                                << except::tendril_key(b_tend->first)
                                << except::cell_name(m->name()));
        }
      }

      ++begin;
    }
  }



  void plasm::save(std::ostream& out) const
  {
    boost::archive::text_oarchive oa(out);
    oa << *this;
  }

  void plasm::load(std::istream& in)
  {
    boost::archive::text_iarchive ia(in);
    ia >> *this;
  }
}

