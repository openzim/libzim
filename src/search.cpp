/*
 * Copyright (C) 2007 Tommi Maekitalo
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

#include <zim/search.h>
#include <zim/fileiterator.h>
#include <zim/indexarticle.h>
#include <zim/files.h>
#include <sstream>
#include <cxxtools/log.h>
#include <map>
#include <math.h>
#include <cctype>
#include <stdexcept>

log_define("zim.search")

namespace zim
{
  namespace
  {
    class PriorityGt : public std::binary_function<bool, SearchResult, SearchResult>
    {
      public:
        bool operator() (const SearchResult& s1, const SearchResult& s2) const
        {
          return s1.getPriority() > s2.getPriority()
              || s1.getPriority() == s2.getPriority()
               && s1.getArticle().getTitle() > s2.getArticle().getTitle();
        }
    };
  }

  double SearchResult::getPriority() const
  {
    if (!wordList.empty() && priority == 0.0)
    {
      log_debug("weightOcc=" << Search::getWeightOcc()
            << " weightPlus=" << Search::getWeightPlus()
            << " weightOccOff=" << Search::getWeightOccOff()
            << " weightDist=" << Search::getWeightDist()
            << " weightPos=" << Search::getWeightPos()
            << " weightDistinctWords=" << Search::getWeightDistinctWords());

      priority = 1.0;

      log_debug("getPriority, " << wordList.size() << " words");

      // weight occurencies of words in article and title
      for (WordListType::const_iterator itw = wordList.begin(); itw != wordList.end(); ++itw)
      {
        priority *= 1.0 + log(itw->second.count * Search::getWeightOcc()
                                + Search::getWeightPlus() * itw->second.addweight)
                        + Search::getWeightOccOff()
                        + Search::getWeightPlus() * itw->second.addweight;

        std::string title = article.getTitle().toUtf8();
        for (std::string::iterator it = title.begin(); it != title.end(); ++it)
          *it = std::tolower(*it);

        //std::string::size_type p = title.find(itw->first);
        //if (p != std::string::npos)
          //priority *= Search::getWeightTitle() / (p + 1) / title.size();
      }

      log_debug("priority1: " << priority);

      // weight distinct words
      priority += Search::getWeightDistinctWords() * wordList.size();

      log_debug("priority2: " << priority);

      // weight distance between different words
      PosListType::const_iterator itp = posList.begin();
      std::string word = itp->second;
      uint32_t pos = itp->first + word.size();
      for (++itp; itp != posList.end(); ++itp)
      {
        if (word != itp->second)
        {
          uint32_t dist = itp->first > pos ? (itp->first - pos)
                        : itp->first < pos ? (pos - itp->first)
                        : 1;
          priority += Search::getWeightDist() / dist;
        }
        word = itp->second;
        pos = itp->first + word.size();
      }

      log_debug("priority3: " << priority);

      // weight position of words in the document
      for (itp = posList.begin(); itp != posList.end(); ++itp)
        priority += Search::getWeightPos() * itp->first / article.getData().size();

      log_debug("priority of article " << article.getIndex() << " \"" << article.getTitle() << "\", " << wordList.size() << " words: " << priority);
    }

    return priority;
  }

  void SearchResult::foundWord(const std::string& word, uint32_t pos, unsigned addweight)
  {
    ++wordList[word].count;
    wordList[word].addweight += addweight;
    posList[pos] = word;
  }

  double Search::weightTitle = 10.0;
  double Search::weightOcc = 10.0;
  double Search::weightOccOff = 1.0;
  double Search::weightPlus = 10.0;
  double Search::weightDist = 10;
  double Search::weightPos = 2;
  double Search::weightDistinctWords = 50;
  unsigned Search::searchLimit = 10000;

  Search::Search(Files& files)
  {
    for (Files::iterator it = files.begin(); it != files.end(); ++it)
    {
      if (it->second.hasNamespace('A'))
      {
        log_debug("articlefile " << it->second.getFilename());
        articlefile = it->second;
      }

      if (it->second.hasNamespace('X'))
      {
        log_debug("indexfile " << it->second.getFilename());
        indexfile = it->second;
      }
    }

    if (!articlefile.good())
      throw std::runtime_error("no article file found");
  }

  void Search::search(Results& results, const std::string& expr)
  {
    std::istringstream ssearch(expr);
    std::string token;

    // map from article-idx to article + relevance-informations
    typedef std::map<uint32_t, SearchResult> IndexType;
    IndexType index;

    while (ssearch >> token)
    {
      unsigned addweight = 0;
      while (token.size() > 0 && token.at(0) == '+')
      {
        ++addweight;
        token.erase(0, 1);
      }

      for (std::string::iterator it = token.begin(); it != token.end(); ++it)
        *it = std::tolower(*it);

      log_debug("search for token \"" << token << '"');

      IndexArticle indexarticle = indexfile.getArticle('X', QUnicodeString::fromUtf8(token), true);

      for (unsigned cat = 0; cat < 4; ++cat)
      {
        const IndexArticle::EntriesType ent = indexarticle.getCategory(cat);
        for (IndexArticle::EntriesType::const_iterator it = ent.begin(); it != ent.end(); ++it)
        {
          uint32_t articleIdx = it->index;
          uint32_t position = it->pos;

          IndexType::iterator itIt = index.insert(
            IndexType::value_type(articleIdx,
              SearchResult(articlefile.getArticle(articleIdx)))).first;

          itIt->second.foundWord(token, position, addweight + 3 - cat);
        }
      }
    }

    log_debug("copy/filter " << index.size() << " articles");
    results.setExpression(expr);
    for (IndexType::const_iterator it = index.begin(); it != index.end(); ++it)
    {
      if (it->second.getCountPositions() > 1)
        results.push_back(it->second);
      else
        log_debug("discard article " << it->first);
    }

    if (results.empty())
    {
      for (IndexType::const_iterator it = index.begin(); it != index.end(); ++it)
        results.push_back(it->second);
    }

    log_debug("sort " << results.size() << " articles");
    std::sort(results.begin(), results.end(), PriorityGt());
  }

  void Search::find(Results& results, char ns, const QUnicodeString& praefix, unsigned limit)
  {
    log_debug("find results in namespace " << ns << " for praefix \"" << praefix << '"');
    for (File::const_iterator pos = articlefile.find(ns, praefix, true);
         pos != articlefile.end() && results.size() < limit; ++pos)
    {
      if (ns != pos->getNamespace() || pos->getTitle().compareCollate(0, praefix.size(), praefix) > 0)
      {
        log_debug("article " << pos->getNamespace() << ", \"" << pos->getTitle() << "\" does not match " << ns << ", \"" << praefix << '"');
        break;
      }
      results.push_back(SearchResult(*pos));
    }
    log_debug(results.size() << " articles in result");
  }

  void Search::find(Results& results, char ns, const QUnicodeString& begin,
    const QUnicodeString& end, unsigned limit)
  {
    log_debug("find results in namespace " << ns << " for praefix \"" << begin << '"');
    for (File::const_iterator pos = articlefile.find(ns, begin, true);
         pos != articlefile.end() && results.size() < limit; ++pos)
    {
      log_debug("check " << pos->getNamespace() << '/' << pos->getTitle());
      if (pos->getNamespace() != ns || pos->getTitle().compareCollate(0, end.size(), end) > 0)
      {
        log_debug("article \"" << pos->getUrl() << "\" does not match");
        break;
      }
      results.push_back(SearchResult(*pos));
    }
    log_debug(results.size() << " articles in result");
  }

}
