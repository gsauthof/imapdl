// Copyright 2014, Georg Sauthoff <mail@georg.so>

/* {{{ GPLv3

    This file is part of imapdl.

    imapdl is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    imapdl is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with imapdl.  If not, see <http://www.gnu.org/licenses/>.

}}} */
#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <boost/log/support/exception.hpp>

// Replacement for BOOST_THROW_EXCEPTION
//   Difference: it also includes ::boost::log::current_scope()

#define THROW_LOG_EXCEPTION(x)\
          ::boost::throw_exception( ::boost::enable_error_info(x) <<\
                      ::boost::throw_function(BOOST_THROW_EXCEPTION_CURRENT_FUNCTION) <<\
                      ::boost::throw_file(__FILE__) <<\
                      ::boost::throw_line((int)__LINE__) <<\
                      ::boost::log::current_scope() )

#define THROW_LOGIC_MSG(S) \
  THROW_LOG_EXCEPTION( \
    std::logic_error(S) \
  )

#define THROW_MSG(S) \
  THROW_LOG_EXCEPTION( \
    std::runtime_error(S) \
  )

// const boost::system::error_code &EC
#define THROW_ERROR(EC) THROW_MSG(EC.message())

#endif
