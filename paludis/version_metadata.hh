/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2005, 2006, 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
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

#ifndef PALUDIS_GUARD_PALUDIS_VERSION_METADATA_HH
#define PALUDIS_GUARD_PALUDIS_VERSION_METADATA_HH 1

#include <paludis/name.hh>
#include <paludis/util/instantiation_policy.hh>
#include <paludis/util/private_implementation_pattern.hh>
#include <paludis/util/sr.hh>
#include <paludis/dep_spec.hh>
#include <paludis/package_database_entry.hh>
#include <string>

/** \file
 * Declarations for the VersionMetadata class.
 *
 * \ingroup grpversions
 */

namespace paludis
{
    class VersionMetadataEbuildInterface;
    class VersionMetadataEbinInterface;
    class VersionMetadataCRANInterface;
    class VersionMetadataDepsInterface;
    class VersionMetadataOriginsInterface;
    class VersionMetadataVirtualInterface;
    class VersionMetadataLicenseInterface;
    class VersionMetadata;

    /**
     * Implemented as a virtual base by VersionMetadata descendents, to allow
     * interfaces to access the base VersionMetadata class.
     *
     * \ingroup grpversions
     */
    class PALUDIS_VISIBLE VersionMetadataHasInterfaces
    {
        protected:
            ///\name Basic operations
            ///\{

            VersionMetadataHasInterfaces();

        public:
            virtual ~VersionMetadataHasInterfaces();

            ///\}

            /**
             * Fetch our base VersionMetadata class.
             */
            virtual const VersionMetadata * version_metadata() const = 0;
    };

    /**
     * A pointer to a parse function for package dependencies.
     *
     * \ingroup grpversions
     */
    typedef std::tr1::shared_ptr<const CompositeDepSpec> (* DepParserFunction) (const std::string &,
            const PackageDepSpecParseMode);

    /**
     * A pointer to a parse function for text.
     *
     * \ingroup grpversions
     */
    typedef std::tr1::shared_ptr<const CompositeDepSpec> (* TextParserFunction) (const std::string &);

#include <paludis/version_metadata-sr.hh>

    /**
     * Version metadata.
     *
     * \ingroup grpversions
     */
    class PALUDIS_VISIBLE VersionMetadata :
        private InstantiationPolicy<VersionMetadata, instantiation_method::NonCopyableTag>,
        public VersionMetadataBase,
        public VersionMetadataCapabilities
    {
        public:
            virtual ~VersionMetadata();

            /**
             * Return the appropriate PackageDepSpecParseMode for our EAPI.
             */
            PackageDepSpecParseMode eapi_as_package_dep_spec_parse_mode() const;

            /**
             * Return the appropriate IUseFlagParseMode for our EAPI.
             */
            IUseFlagParseMode eapi_as_iuse_flag_parse_mode() const;

        protected:
            ///\name Basic operations
            ///\{

            VersionMetadata(const VersionMetadataBase::NamedArguments<> &, const VersionMetadataCapabilities &);

            ///\}
    };
}

#endif
