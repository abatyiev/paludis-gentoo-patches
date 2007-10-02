/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
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

#ifndef PALUDIS_GUARD_PALUDIS_REPOSITORIES_VIRTUALS_PACKAGE_ID_HH
#define PALUDIS_GUARD_PALUDIS_REPOSITORIES_VIRTUALS_PACKAGE_ID_HH 1

#include <paludis/util/private_implementation_pattern.hh>
#include <paludis/package_id.hh>
#include <paludis/metadata_key.hh>

namespace paludis
{
    namespace virtuals
    {
        class VirtualsPackageIDKey :
            public MetadataPackageIDKey,
            private PrivateImplementationPattern<VirtualsPackageIDKey>
        {
            private:
                Implementation<VirtualsPackageIDKey> * const _imp;

            public:
                VirtualsPackageIDKey(const tr1::shared_ptr<const PackageID> &);
                ~VirtualsPackageIDKey();

                virtual const tr1::shared_ptr<const PackageID> value() const
                    PALUDIS_ATTRIBUTE((warn_unused_result));
        };

        class VirtualsDepKey :
            public MetadataSpecTreeKey<DependencySpecTree>,
            private PrivateImplementationPattern<VirtualsDepKey>
        {
            private:
                Implementation<VirtualsDepKey> * const _imp;

            public:
                VirtualsDepKey(const Environment * const, const std::string &, const std::string &,
                        const tr1::shared_ptr<const PackageID> &, const bool);
                ~VirtualsDepKey();

                virtual const tr1::shared_ptr<const DependencySpecTree::ConstItem> value() const
                    PALUDIS_ATTRIBUTE((warn_unused_result));

                virtual std::string pretty_print(const DependencySpecTree::Formatter &) const
                    PALUDIS_ATTRIBUTE((warn_unused_result));

                virtual std::string pretty_print_flat(const DependencySpecTree::Formatter &) const
                    PALUDIS_ATTRIBUTE((warn_unused_result));
        };

        class VirtualsPackageID :
            private PrivateImplementationPattern<VirtualsPackageID>,
            public PackageID
        {
            private:
                Implementation<VirtualsPackageID> * const _imp;

            protected:
                virtual void need_keys_added() const;
                virtual void need_masks_added() const;

            public:
                VirtualsPackageID(
                        const Environment * const,
                        const tr1::shared_ptr<const Repository> & repo,
                        const QualifiedPackageName & virtual_name,
                        const tr1::shared_ptr<const PackageID> & virtual_for,
                        const bool exact);

                virtual ~VirtualsPackageID();

                virtual const std::string canonical_form(const PackageIDCanonicalForm f) const;

                virtual const QualifiedPackageName name() const;
                virtual const VersionSpec version() const;
                virtual const SlotName slot() const;
                virtual const tr1::shared_ptr<const Repository> repository() const;

                virtual const tr1::shared_ptr<const MetadataPackageIDKey> virtual_for_key() const;
                virtual const tr1::shared_ptr<const MetadataSetKey<KeywordNameSet> > keywords_key() const;
                virtual const tr1::shared_ptr<const MetadataSetKey<IUseFlagSet> > iuse_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<LicenseSpecTree> > license_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<ProvideSpecTree> > provide_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> > build_dependencies_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> > run_dependencies_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> > post_dependencies_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> > suggested_dependencies_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<FetchableURISpecTree> > src_uri_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<SimpleURISpecTree> > homepage_key() const;
                virtual const tr1::shared_ptr<const MetadataStringKey> short_description_key() const;
                virtual const tr1::shared_ptr<const MetadataStringKey> long_description_key() const;
                virtual const tr1::shared_ptr<const MetadataContentsKey> contents_key() const;
                virtual const tr1::shared_ptr<const MetadataTimeKey> installed_time_key() const;
                virtual const tr1::shared_ptr<const MetadataStringKey> source_origin_key() const;
                virtual const tr1::shared_ptr<const MetadataStringKey> binary_origin_key() const;
                virtual const tr1::shared_ptr<const MetadataSetKey<PackageIDSequence> > contains_key() const;
                virtual const tr1::shared_ptr<const MetadataPackageIDKey> contained_in_key() const;
                virtual const tr1::shared_ptr<const MetadataFSEntryKey> fs_location_key() const;

                virtual bool supports_action(const SupportsActionTestBase &) const PALUDIS_ATTRIBUTE((warn_unused_result));
                virtual void perform_action(Action &) const;

                virtual bool arbitrary_less_than_comparison(const PackageID &) const
                    PALUDIS_ATTRIBUTE((warn_unused_result));

                virtual std::size_t extra_hash_value() const
                    PALUDIS_ATTRIBUTE((warn_unused_result));

                virtual bool breaks_portage() const PALUDIS_ATTRIBUTE((warn_unused_result));

                virtual void invalidate_masks() const;
        };
    }
}

#endif
