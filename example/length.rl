# Copyright 2014, Georg Sauthoff <mail@georg.so>

# {{{ GPLv3
#
#   This file is part of imapdl.
#
#   imapdl is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   imapdl is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with imapdl.  If not, see <http://www.gnu.org/licenses/>.
#
# }}}

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cassert>

using namespace std;

#include <boost/lexical_cast.hpp>

#include <buffer/buffer.h>


%%{

machine ma;


action number_begin
{
  buffer.start(p);
  number = 0;
}
action number_end
{
  buffer.finish(p);
  number = boost::lexical_cast<unsigned>(string(buffer.begin(), buffer.end()));
  buffer.clear();
  literal_pos = 0;
  cout << "number: " << number << '\n';
}
action call_literal_tail
{
  if (number) {
    cout << ">>call\n";
    fcall literal_tail;
  } else {
    cout << "not calling because of zero length string\n";
  }
}
action literal_tail_begin
{
  buffer.start(p);
}
action literal_tail_cond_return
{
  cout << fc << '\n';
  ++literal_pos;
  if (literal_pos == number) {
    buffer.finish(p+1);
    cout << ">>|";
    cout.write(buffer.data(), buffer.end()-buffer.begin());
    cout << "|\n";
    cout << "ret\n";
    fret;
  }
}


CHAR8 = [^\0] ;
SP = ' ' ;
number = [0-9]+;

#literal = '{' number '}' CRLF CHAR8* ;

literal_tail := (CHAR8*) >literal_tail_begin  $literal_tail_cond_return ;
literal = '{'  number >number_begin %number_end '}' '_' @call_literal_tail ;

main :=  SP literal+ SP ;

prepush {
  if (unsigned(top) == v.size()) {
    v.push_back(0);
    stack = v.data();
  }
}

}%%

%% write data;

int main( int argc, char **argv )
{
  if (argc < 2) {
    cerr << "not enough arguments!\n";
    return 1;
  }
  int cs = 0;
  vector<int> v;

  using namespace Memory;
  Buffer::Vector buffer;
  size_t number = 0;
  size_t literal_pos = 0;

  int *stack = v.data();
  int top = 0;
  %% write init;

  for (int i = 1; i<argc; ++i) {
    const char *p = argv[i];
    const char *pe = p + strlen(p) ;
    const char *eof = i+1 == argc ? pe : 0;
    (void)eof;
    buffer.resume(p);
    %% write exec;
    if (cs == %%{write error;}%%)
      break;
    buffer.stop(pe);
  }

  if (cs == %%{write error;}%%)
    cout << "Lexer ERROR\n";
  if (cs < %%{write first_final;}%%)
    cout << "Lexer not in final state\n";
  if (top)
    cout << "Stack is not empty: " << top << '\n';
  cout << "exit (in state=" << cs << ")\n";
  return 0;
}

