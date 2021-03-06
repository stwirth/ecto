/*
 * Copyright (c) 2011, Willow Garage, Inc.
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
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
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
#pragma once


#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include <ecto/forward.hpp>
#include <ecto/tendril.hpp>
#include <ecto/tendrils.hpp>
#include <ecto/strand.hpp>
#include <ecto/util.hpp>
#include <ecto/profile.hpp>
#include <ecto/traits.hpp>

namespace ecto
{

  /**
   * \brief Return values for modules' process functions. These
   * are appropriate for non exceptional behavior.
   */
  enum ReturnCode
  {
    OK = 0, //!< Everything A OK.
    QUIT = 1, //!< Explicit quit now.
    BREAK = 2, //!< Stop execution in my scope, jump to outer scope
    CONTINUE = 3, //!< Stop execution in my scope, jump to top of scope
    UNKNOWN = -1 //!< Unknown return code.
  };

#define ECTO_RETURN_VALUES                                                \
    (OK)(QUIT)(CONTINUE)(BREAK)(UNKNOWN)                                  \

  const std::string&
  ReturnCodeToStr(int rval);

  /**
   * \brief ecto::cell is the non virtual interface to the basic building
   * block of ecto graphs.  This interface should never be the parent of
   * client cell, but may be used for polymorphic access to client cells.
   *
   * Clients should expose their code to this interface through
   * ecto::wrap, or ecto::create_cell<T>().
   *
   * For a client's cell to satisfy the ecto::cell idium, it must
   * look similar to the following definition.
   * @code
   struct MyEctoCell
   {
     //called first thing, the user should declare their parameters in this
     //free standing function.
     static void declare_params(tendrils& params);
     //declare inputs and outputs here. The parameters may be used to
     //determine the io
     static void declare_io(const tendrils& params, tendrils& in, tendrils& out);
     //called right after allocation of the cell, exactly once.
     void configure(tendrils& params, tendrils& inputs, tendrils& outputs);
     //called at every execution of the graph
     int process(const tendrils& in, tendrils& out);
   };
   * @endcode
   *
   * It is important to note that all functions have are optional and they all have
   * default implementations.
   */
  struct ECTO_EXPORT cell: boost::noncopyable
  {
    typedef boost::shared_ptr<cell> ptr; //!< A convenience pointer typedef

    cell();
    virtual ~cell();

    /**
     * \brief Dispatches parameter declaration code. After this code, the parameters
     * for the cell will be set to their defaults.
     */
    void declare_params();
    /**
     * \brief Dispatches input/output declaration code.  It is assumed that the parameters
     * have been declared before this is called, so that inputs and outputs may be dependent
     * on those parameters.
     */
    void declare_io();

    /**
     * \brief Given initialized parameters,inputs, and outputs, this will dispatch the client
     * configuration code.  This will allocated an instace of the clients cell, so this
     * should not be called during introspection.
     */
    void configure();

    /**
       scheduler is going to call process() zero or more times.
     */
    void start();

    /**
       scheduler is not going to call process() for a while.
     */
    void stop();

    /**
     * \brief Dispatches the process function for the client cell.  This should only
     * be called from one thread at a time.
     *
     * Also, this function may throw exceptions...
     *
     * @return A return code, ecto::OK , or 0 means all is ok. Anything non zero should be considered an
     * exit signal.
     */
    ReturnCode process();

    /**
     * \brief Return the type of the child class.
     * @return A human readable non mangled name for the client class.
     */
    std::string type() const;

    /**
     * \brief Grab the name of the instance.
     * @return The name of the instance, or the address if none was given when object was constructed
     */
    std::string name() const;

    /**
     * \brief Set the name of the instance.
     */
    void name(const std::string&);

    /**
     * \brief Set the short_doc_ of the instance.
     */
    std::string short_doc() const;

    /**
     * \brief Set the short_doc_ of the instance.
     */
    void short_doc(const std::string&);

    void reset_strand();
    void set_strand(ecto::strand);

    /**
     * \brief Generate an Restructured Text doc string for the cell. Includes documentation for all parameters,
     * inputs, outputs.
     * @param doc The highest level documentation for the cell.
     * @return A nicely formatted doc string.
     */
    std::string gen_doc(const std::string& doc = "A module...") const;

    void verify_params() const;
    void verify_inputs() const;

    ptr clone() const;

    tendrils parameters; //!< Parameters
    tendrils inputs; //!< Inputs, inboxes, always have a valid value ( may be NULL )
    tendrils outputs; //!< Outputs, outboxes, always have a valid value ( may be NULL )

    boost::optional<strand> strand_; //!< The strand that this cell should be executed in.
    profile::stats_type stats; //!< For collecting execution statistics for process.

    virtual bool init() = 0;

    std::size_t tick() const;
    void inc_tick();
    void reset_tick();

    bool stop_requested() const { return stop_requested_; }
    void stop_requested(bool b) { stop_requested_ = b; }

    boost::signals2::signal<void(cell&, bool)> bsig_process;

  protected:

    virtual void dispatch_declare_params(tendrils& t) = 0;

    virtual void dispatch_declare_io(const tendrils& params, tendrils& inputs,
                                     tendrils& outputs) = 0;

    virtual void dispatch_configure(const tendrils& params, const tendrils& inputs,
                                    const tendrils& outputs) = 0;

    virtual ReturnCode dispatch_process(const tendrils& inputs, const tendrils& outputs) = 0;

    virtual void dispatch_start() = 0;
    virtual void dispatch_stop() = 0;

    virtual std::string dispatch_name() const = 0;

    virtual ptr dispatch_clone() const = 0;

    virtual std::string dispatch_short_doc() const
    {
      return "";
    }

    virtual void dispatch_short_doc(const std::string&) { }

  private:

    cell(const cell&);

    std::string instance_name_;
    bool stop_requested_;
    bool configured;
    std::size_t tick_;
    boost::mutex mtx;
#if defined(ECTO_STRESS_TEST)
    boost::mutex process_mtx;
#endif

    friend struct ecto::schedulers::access;
  };


  /**
   * \brief Helper class for determining if client modules have function
   * implementations or not.
   * @internal
   */
  template<class T>
  struct has_f
  {
    typedef char yes;
    typedef char (&no)[2];

    // SFINAE eliminates this when the type of arg is invalid
    // overload resolution prefers anything at all over "..."
    template<class U>
    static yes test_declare_params(__typeof__(&U::declare_params));
    template<class U>
    static no test_declare_params(...);
    enum
    {
      declare_params = sizeof(test_declare_params<T> (0)) == sizeof(yes)
    };

    template<class U>
    static yes test_declare_io(__typeof__(&U::declare_io));
    template<class U>
    static no test_declare_io(...);
    enum
    {
      declare_io = sizeof(test_declare_io<T> (0)) == sizeof(yes)
    };

    template<class U>
    static yes test_configure(__typeof__(&U::configure));
    template<class U>
    static no test_configure(...);
    enum
    {
      configure = sizeof(test_configure<T> (0)) == sizeof(yes)
    };

    template<class U>
    static yes test_process(__typeof__(&U::process));
    template<class U>
    static no test_process(...);
    enum
    {
      process = sizeof(test_process<T> (0)) == sizeof(yes)
    };

    template<class U>
    static yes test_start(__typeof__(&U::start));
    template<class U>
    static no test_start(...);
    enum
    {
      start = sizeof(test_start<T> (0)) == sizeof(yes)
    };

    template<class U>
    static yes test_stop(__typeof__(&U::stop));
    template<class U>
    static no test_stop(...);
    enum
    {
      stop = sizeof(test_stop<T> (0)) == sizeof(yes)
    };
  };

  /**
   * \brief cell_<T> is for registering an arbitrary class
   * with the the cell NVI. This adds a barrier between client code and the cell.
   */
  template<class Impl>
  struct cell_: cell
  {
    typedef boost::shared_ptr<cell_<Impl> > ptr;

    typedef typename detail::python_mutex<Impl>::type gil_mtx_t;

    cell_() {
      init_strand(typename ecto::detail::is_threadsafe<Impl>::type());
    }

    ~cell_() { }
    template <int I> struct int_ { };
    typedef int_<0> not_implemented;
    typedef int_<1> implemented;

    //
    // declare_params
    //
    typedef int_<has_f<Impl>::declare_params> has_declare_params;

    static void declare_params(tendrils& params, not_implemented) { }

    static void declare_params(tendrils& params, implemented)
    {
      Impl::declare_params(params);
    }

    static void declare_params(tendrils& params)
    {
      declare_params(params, has_declare_params());
    }

    void dispatch_declare_params(tendrils& params)
    {
      declare_params(params);
    }


    //
    // declare_io
    //
    static void declare_io(const tendrils& params, tendrils& inputs, tendrils& outputs, not_implemented)
    { }

    static void declare_io(const tendrils& params, tendrils& inputs, tendrils& outputs, implemented)
    {
      Impl::declare_io(params, inputs, outputs);
    }

    typedef int_<has_f<Impl>::declare_io> has_declare_io;

    static void declare_io(const tendrils& params, tendrils& inputs, tendrils& outputs)
    {
      declare_io(params, inputs, outputs, has_declare_io());
    }

    void dispatch_declare_io(const tendrils& params, tendrils& inputs,
                             tendrils& outputs)
    {
      declare_io(params, inputs, outputs);
    }

    //
    // configure
    //
    void configure(const tendrils&, const tendrils& , const tendrils&, not_implemented)
    {
    }

    void configure(const tendrils& params, const tendrils& inputs, const tendrils& outputs,
                   implemented)
    {
      impl->configure(params,inputs,outputs);
    }

    void dispatch_configure(const tendrils& params, const tendrils& inputs,
                            const tendrils& outputs)
    {
      configure(params, inputs, outputs, int_<has_f<Impl>::configure> ());
    }

    //
    // process
    //
    ReturnCode process(const tendrils&, const tendrils&, not_implemented)
    {
      return OK;
    }

    ReturnCode process(const tendrils& inputs, const tendrils& outputs, implemented)
    {
      return ReturnCode(impl->process(inputs, outputs));
    }

    ReturnCode dispatch_process(const tendrils& inputs, const tendrils& outputs)
    {
      return process(inputs, outputs, int_<has_f<Impl>::process> ());
    }

    //
    // start
    //
    void start(not_implemented) { }
    void start(implemented) { impl->start(); }
    void dispatch_start()
    {
      start(int_<has_f<Impl>::start> ());
    }

    //
    // stop
    //
    void stop(not_implemented) { }
    void stop(implemented) { impl->stop(); }
    void dispatch_stop()
    {
      stop(int_<has_f<Impl>::stop> ());
    }

    std::string dispatch_name() const
    {
      return CELL_TYPE_NAME;
    }
    std::string dispatch_short_doc() const
    {
      return SHORT_DOC;
    }

    void dispatch_short_doc(const std::string&) { }

    cell::ptr dispatch_clone() const
    {
      return cell::ptr(new cell_<Impl> ());
    }

    bool init()
    {
      try {
        if(!impl)
        {
          impl.reset(new Impl);
          Impl* i=impl.get();
          //these handle finalizing the registration of spores that
          //were registered at static time.
          parameters.realize_potential(i);
          inputs.realize_potential(i);
          outputs.realize_potential(i);
        }
        return impl;
      }
      catch (const std::exception& e)
      {
        ECTO_TRACE_EXCEPTION("const std::exception&");
        BOOST_THROW_EXCEPTION(except::CellException()
                              << except::when("Construction")
                              << except::type(name_of(typeid(e)))
                              << except::cell_name(name())
                              << except::what(e.what()));
      }
      catch (...)
      {
        ECTO_TRACE_EXCEPTION("...");
        BOOST_THROW_EXCEPTION(except::CellException()
                              << except::when("Construction")
                              << except::what("(unknown exception)")
                              << except::cell_name(name()));
      }
    }

  public:

    boost::shared_ptr<Impl> impl;
    static std::string SHORT_DOC;
    static std::string CELL_NAME; //!< The python name for the cell.
    static std::string MODULE_NAME; //!< The module that the cell is part of.
    static const std::string CELL_TYPE_NAME;

  private:

    void init_strand(boost::mpl::true_)
    {
    } // threadsafe

    void init_strand(boost::mpl::false_) {
      static ecto::strand strand_;
      cell::strand_ = strand_;
      ECTO_ASSERT(cell::strand_->id() == strand_.id(), "Catastrophe... cells not correctly assignable");
      ECTO_LOG_DEBUG("%s cell has strand id=%p", cell::type() % cell::strand_->id());
    }
  };

  template<typename Impl>
  std::string cell_<Impl>::SHORT_DOC;

  template<typename Impl>
  std::string cell_<Impl>::CELL_NAME;

  template<typename Impl>
  std::string cell_<Impl>::MODULE_NAME;

  template<typename Impl>
  const std::string cell_<Impl>::CELL_TYPE_NAME = ecto::name_of<Impl>();

}//namespace ecto

