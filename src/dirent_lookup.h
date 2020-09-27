/*
 * Copyright (C) 2020 Veloman Yunkan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * is provided AS IS, WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, and
 * NON-INFRINGEMENT.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#ifndef ZIM_DIRENT_LOOKUP_H
#define ZIM_DIRENT_LOOKUP_H

#include "zim_types.h"
#include "debug.h"

namespace zim
{

template<class Impl>
class DirentLookup
{
public: // types
  struct DirentRange
  {
    article_index_t begin, end;
  };

  typedef std::pair<bool, article_index_t> Result;

public: // functions
  explicit DirentLookup(Impl* _impl) : impl(*_impl) {}

  DirentRange getDirentRange(char ns, const std::string& url) const;

  Result find(char ns, const std::string& url);

private: // data
  Impl& impl;
};

template<class Impl>
typename DirentLookup<Impl>::DirentRange
DirentLookup<Impl>::getDirentRange(char ns, const std::string& /*url*/) const
{
  DirentRange r;
  r.begin = impl.getNamespaceBeginOffset(ns);
  r.end   = impl.getNamespaceEndOffset(ns);
  return r;
}

template<typename Impl>
typename DirentLookup<Impl>::Result
DirentLookup<Impl>::find(char ns, const std::string& url)
{
  const DirentRange r = getDirentRange(ns, url);
  article_index_type l(r.begin);
  article_index_type u(r.end);

  if (l == u)
  {
    return std::pair<bool, article_index_t>(false, article_index_t(0));
  }

  unsigned itcount = 0;
  while (u - l > 1)
  {
    ++itcount;
    article_index_type p = l + (u - l) / 2;
    auto d = impl.getDirent(article_index_t(p));

    int c = ns < d->getNamespace() ? -1
          : ns > d->getNamespace() ? 1
          : url.compare(d->getUrl());

    if (c < 0)
      u = p;
    else if (c > 0)
      l = p;
    else
    {
      return std::pair<bool, article_index_t>(true, article_index_t(p));
    }
  }

  auto d = impl.getDirent(article_index_t(l));
  int c = url.compare(d->getUrl());

  if (c == 0)
  {
    return std::pair<bool, article_index_t>(true, article_index_t(l));
  }

  return std::pair<bool, article_index_t>(false, article_index_t(c < 0 ? l : u));
}

template<typename IMPL>
article_index_t getNamespaceBeginOffset(IMPL& impl, char ch)
{
  ASSERT(ch, >=, 32);
  ASSERT(ch, <=, 127);

  article_index_type lower = 0;
  article_index_type upper = article_index_type(impl.getCountArticles());
  auto d = impl.getDirent(article_index_t(0));
  while (upper - lower > 1)
  {
    article_index_type m = lower + (upper - lower) / 2;
    auto d = impl.getDirent(article_index_t(m));
    if (d->getNamespace() >= ch)
      upper = m;
    else
      lower = m;
  }

  article_index_t ret = article_index_t(d->getNamespace() < ch ? upper : lower);
  return ret;
}

template<typename IMPL>
article_index_t getNamespaceEndOffset(IMPL& impl, char ch)
{
  ASSERT(ch, >=, 32);
  ASSERT(ch, <, 127);
  return getNamespaceBeginOffset(impl, ch+1);
}


} // namespace zim

#endif // ZIM_DIRENT_LOOKUP_H
