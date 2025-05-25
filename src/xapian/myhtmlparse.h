/* myhtmlparse.h: subclass of HtmlParser for extracting text
 *
 * Copyright 1999,2000,2001 BrightStation PLC
 * Copyright 2002,2003,2004,2006,2008 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#ifndef OMEGA_INCLUDED_MYHTMLPARSE_H
#define OMEGA_INCLUDED_MYHTMLPARSE_H

#include <zim/zim.h>
#include "htmlparse.h"

// FIXME: Should we include \xa0 which is non-breaking space in iso-8859-1, but
// not in all charsets and perhaps spans of all \xa0 should become a single
// \xa0?
#define WHITESPACE " \t\n\r"

namespace zim {

class LIBZIM_PRIVATE_API MyHtmlParser : public HtmlParser {
    public:
	bool in_script_tag;
	bool in_style_tag;
	bool pending_space;
	bool indexing_allowed;
	bool charset_from_meta;
    float latitude, longitude;
    bool has_geoPosition;
	string title, sample, keywords, dump;
	void process_text(const string &text);
	void opening_tag(const string &tag);
	void closing_tag(const string &tag);
	using HtmlParser::parse_html;
	void parse_html(const string &text, const string &charset_,
			bool charset_from_meta_);
	MyHtmlParser() :
		in_script_tag(false),
		in_style_tag(false),
		pending_space(false),
		indexing_allowed(true),
		charset_from_meta(false),
        latitude(0), longitude(0), has_geoPosition(false) { }

	void reset() {
	    in_script_tag = false;
	    in_style_tag = false;
	    pending_space = false;
	    indexing_allowed = true;
	    charset_from_meta = false;
        latitude = longitude = 0;
        has_geoPosition = false;
	    title.resize(0);
	    sample.resize(0);
	    keywords.resize(0);
	    dump.resize(0);
	}
};

};

#endif // OMEGA_INCLUDED_MYHTMLPARSE_H
