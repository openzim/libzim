Libzim programming
==================

Introduction
------------

libzim is written in C++. To use the library, you need the include files of libzim have
to link against libzim.

Errors are handled with exceptions. When something goes wrong, libzim throws an error,
which is always derived from std::exception.

All classes are defined in the namespace zim.
Copying is allowed and tried to make as cheap as possible.
The reading part of the libzim is most of the time thread safe.
Searching and creating part are not. You have to serialize access to the class yourself.

The main class, which accesses a archive is |Archive|.
It has actually a reference to an implementation, so that copies of the class just references the same file.
You open a file by passing the file name to the constructor as a std::string.

Iterating over entries is made by creating a |EntryRange|.

.. code-block:: c++

    #include <zim/file.h>
    #include <zim/fileiterator.h>
    #include <iostream>
    int main(int argc, char* argv[])
    {
      try
      {
        zim::Archive a("wikipedia.zim");

        for (auto entry: a.iterByPath()) {
          std::cout << "path: " << entry.getPath() << " title: " << entry.getTitle() << std::endl;
        }
      } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
      }
    }

In subsequent examples, only code needed to use the library will be explained.
The main-function with the error catcher should always be in place.

Getting entries
---------------

Entries are addressed either by path or title.

|Archive| has methods |getEntryByPath| and |getEntryByTitle|. Both take 1 parameters : a string, which specifies the path or the title of the entry to get.
They return a |Entry|.
If the entry cannot be found, they throw the exception |EntryNotFound|.

Entry are entry point in a archive for "things". It can be a redirection to another entry or a |Item|

  .. code-block:: c++

    auto entry = archive.getEntryByPath("foo");
    if (entry.isRedirect()) {
      std::cout << "This is a redirection to " << entry.getRedirectEntry().getPath() << std::endl();
    } else {
      std::cout << "This is a item with content : " << entry.getItem().getData() << std::endl();
    }

As it is pretty common to resolve potential entry redirection and get the final item, you can do it directly using `getItem` :

  .. code-block:: c++

    auto entry = archive.getEntryByPath("foo");
    auto item = entry.getItem(true);
    if (entry.isRedirect()) {
      std::cout << "Entry " << entry.getPath() << " is a entry pointing to the item " << item.getPath() << std::endl;
    } else {
      std::cout << entry.getPath() << " should be equal to " << item.getPath() << std::endl;
    }
    std::cout << "The item data is " << item.getData() << std::endl;

Finding entries
---------------

|getEntryByPath|/|getEntryByTitle| allow to get a exact entry.
But you may want to find entries using a more loosely method.
|findByPath| and |findByTitle| allow you to find entries starting by the given path/title prefix.

|findByPath|/|findByTitle| return a |EntryRange| you can iterate on :

  .. code-block:: c++

    for (auto entry: archive.findEntryByPath("fo")) {
      std::cout << "Entry " << entry.getPath() << " should starts with fo." << std::endl;
    }

Searching for entries
---------------------

Find entries by path/title is nice but you may want to search for entries base on their content.
If the zim archive contains a full text index, you can search on it.

The class |Searcher| allow to search on one or several |Archive|.
It allows to create a |Search| which represent a particular search for a |Query|.
From a |Search|, you can get a |SearchResultSet| on which you can iterate.

 .. code-block:: c++

    // Create a searcher, something to search on an archive
    zim::Searcher searcher(archive);

    // We need a query to specify what to search for
    zim::Query query;
    query.setQuery("bar");

    // Create a search for the specified query
    zim::Search search = searcher.search(query);

    // Now we can get some result from the search.
    // 20 results starting from offset 10 (from 10 to 30)
    zim::SearchResultSet results = search.getResults(10, 20);

    // SearchResultSet is iterable
    for(auto entry: results) {
      std::cout << entry.getPath() << std::endl;
    }

Searching for suggestions
-------------------------

While |findByTitle| may be a good start to search for suggestion, you may want to search for suggestion for term
in the middle of the suggestion.

The suggestion API allow you to search for suggestion, using suggestion database included in recent zim files.
The suggestion API is pretty close from the search API:

 .. code-block:: c++

    // Create a searcher, something to search on an archive
    zim::SuggestionSearcher searcher(archive);

    // Create a search for the specified query
    zim::SuggestionSearch search = searcher.search("bar");

    // Now we can get some result from the search.
    // 20 results starting from offset 10 (from 10 to 30)
    zim::SuggestionResultSet results = search.getResults(10, 20);

    // SearchResultSet is iterable
    for(auto entry: results) {
      std::cout << entry.getPath() << std::endl;
    }

If the zim file doesn't contain a suggestion database, the suggestion will fallback to |findByTitle| for you.

 .. Declare some replacement helpers

 .. |Archive| replace:: :class:`zim::Archive`
 .. |EntryRange| replace:: :class:`zim::Archive::EntryRange`
 .. |Entry| replace:: :class:`zim::Entry`
 .. |Item| replace:: :class:`zim::Item`
 .. |EntryNotFound| replace:: :class:`zim::EntryNotFound`
 .. |Searcher| replace:: :class:`zim::Searcher`
 .. |Search| replace:: :class:`zim::Search`
 .. |Query| replace:: :class:`zim::Query`
 .. |SearchResultSet| replace:: :class:`zim::SearchResultSet`
 .. |getEntryByPath| replace:: :func:`getEntryByPath<void zim::Archive::getEntryByPath(const std::string&) const>`
 .. |getEntryByTitle| replace:: :func:`getEntryByTitle<void zim::Archive::getEntryByTitle(const std::string&) const>`
 .. |findByPath| replace:: :func:`findByPath<zim::Archive::findByPath>`
 .. |findByTitle| replace:: :func:`findByTitle<zim::Archive::findByTitle>`

