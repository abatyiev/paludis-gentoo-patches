/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006 Ciaran McCreesh <ciaran.mccreesh@blueyonder.co.uk>
 *
 * This file is part of the Paludis package manager. Paludis is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * Paludis is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PALUDIS_GUARD_EBUILD_DIGESTS_MD5_HH
#define PALUDIS_GUARD_EBUILD_DIGESTS_MD5_HH 1

#include <fstream>
#include <iomanip>
#include <string>
#include <inttypes.h>

namespace md5
{
    class MD5
    {
        private:
            static const uint32_t _t[64];
            static const uint8_t _s[64];
            uint32_t _r[4];
            uint64_t _size;
            bool _done_one_pad;

            void _update(const uint8_t * const block);

            inline int _get(std::istream & stream) __attribute__((always_inline))
            {
                char c;
                if (stream.get(c))
                {
                    _size += 8;
                    return static_cast<unsigned char>(c);
                }
                else if (! _done_one_pad)
                {
                    _done_one_pad = true;
                    return 0x80;
                }
                else
                    return -1;
            }

        public:
            MD5(std::istream & stream);

            std::string hexsum() const;
    };

}


#endif
