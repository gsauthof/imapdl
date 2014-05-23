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
#include "sequence_set.h"

using namespace std;

#include <boost/icl/interval_map.hpp>
namespace icl = boost::icl;

using ISet = icl::interval_set<uint32_t, std::less, icl::closed_interval<uint32_t> >;

class Sequence_Set_Priv {
  private:
    ISet iset_;
  public:
    Sequence_Set_Priv();
    ~Sequence_Set_Priv();
    void   push(uint32_t id);
    void   push(uint32_t fst, uint32_t snd);
    void   copy(std::vector<std::pair<uint32_t, uint32_t> > &v) const;
    size_t size() const;
    void   clear();
};

Sequence_Set_Priv::Sequence_Set_Priv()
{
}
Sequence_Set_Priv::~Sequence_Set_Priv()
{
}
void Sequence_Set_Priv::push(uint32_t id)
{
  ISet::interval_type i(id, id);
  iset_.insert(i);
}
void Sequence_Set_Priv::push(uint32_t fst, uint32_t snd)
{
  ISet::interval_type i(fst, snd);
  iset_.insert(i);
}
void Sequence_Set_Priv::copy(std::vector<std::pair<uint32_t, uint32_t> > &v) const
{
  v.clear();
  for (auto &i : iset_)
    v.emplace_back(icl::first(i), icl::last(i));

}
size_t Sequence_Set_Priv::size() const
{
  return icl::interval_count(iset_);
}
void Sequence_Set_Priv::clear()
{
  iset_.clear();
}

Sequence_Set::Sequence_Set()
  : d(new Sequence_Set_Priv())
{
}

Sequence_Set::~Sequence_Set()
{
  delete d;
}

void Sequence_Set::push(uint32_t id)
{
  d->push(id);
}

void Sequence_Set::copy(std::vector<std::pair<uint32_t, uint32_t> > &v) const
{
  d->copy(v);
}

size_t Sequence_Set::size() const
{
  return d->size();
}

void Sequence_Set::clear()
{
  d->clear();
}

Sequence_Set &Sequence_Set::operator=(const std::vector<std::pair<uint32_t, uint32_t> > &v)
{
  for (auto &x : v)
    d->push(x.first, x.second);
  return *this;
}
bool Sequence_Set::empty() const
{
  return size() == 0;
}
