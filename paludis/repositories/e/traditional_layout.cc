/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2005, 2006, 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
 * Copyright (c) 2006 Danny van Dyk <kugelfang@gentoo.org>
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

#include <paludis/repositories/e/traditional_layout.hh>
#include <paludis/repositories/e/e_repository_entries.hh>
#include <paludis/repositories/e/e_repository.hh>
#include <paludis/config_file.hh>
#include <paludis/package_id.hh>
#include <paludis/hashed_containers.hh>
#include <paludis/package_database.hh>
#include <paludis/util/dir_iterator.hh>
#include <paludis/util/fs_entry.hh>
#include <paludis/util/private_implementation_pattern-impl.hh>
#include <paludis/util/log.hh>
#include <paludis/util/stringify.hh>
#include <paludis/util/is_file_with_extension.hh>
#include <paludis/util/iterator.hh>
#include <paludis/util/strip.hh>
#include <paludis/util/sequence.hh>
#include <paludis/util/mutex.hh>
#include <paludis/util/set.hh>

#include <paludis/util/tr1_functional.hh>
#include <functional>
#include <algorithm>
#include <list>

#include <libwrapiter/libwrapiter_forward_iterator.hh>
#include <libwrapiter/libwrapiter_output_iterator.hh>

using namespace paludis;
using namespace paludis::erepository;

typedef MakeHashedMap<CategoryNamePart, bool>::Type CategoryMap;
typedef MakeHashedMap<QualifiedPackageName, bool>::Type PackagesMap;
typedef MakeHashedMap<QualifiedPackageName, tr1::shared_ptr<PackageIDSequence> >::Type IDMap;

namespace paludis
{
    template<>
    struct Implementation<TraditionalLayout>
    {
        const ERepository * const repository;
        const FSEntry tree_root;

        mutable Mutex big_nasty_mutex;

        mutable bool has_category_names;
        mutable CategoryMap category_names;
        mutable PackagesMap package_names;
        mutable IDMap ids;

        mutable tr1::shared_ptr<CategoryNamePartSet> category_names_collection;
        tr1::shared_ptr<const ERepositoryEntries> entries;

        tr1::shared_ptr<FSEntrySequence> arch_list_files;
        tr1::shared_ptr<FSEntrySequence> repository_mask_files;
        tr1::shared_ptr<FSEntrySequence> profiles_desc_files;
        tr1::shared_ptr<FSEntrySequence> mirror_files;
        tr1::shared_ptr<FSEntrySequence> use_desc_dirs;

        Implementation(const ERepository * const r, const FSEntry & t,
                tr1::shared_ptr<const ERepositoryEntries> e) :
            repository(r),
            tree_root(t),
            has_category_names(false),
            entries(e),
            arch_list_files(new FSEntrySequence),
            repository_mask_files(new FSEntrySequence),
            profiles_desc_files(new FSEntrySequence),
            mirror_files(new FSEntrySequence),
            use_desc_dirs(new FSEntrySequence)
        {
        }
    };
}

TraditionalLayout::TraditionalLayout(const ERepository * const repo, const FSEntry & tree_root,
        tr1::shared_ptr<const ERepositoryEntries> e,
        tr1::shared_ptr<const FSEntry> f) :
    Layout(f),
    PrivateImplementationPattern<TraditionalLayout>(new Implementation<TraditionalLayout>(repo, tree_root, e))
{
    if (master_repository_location())
    {
        _imp->arch_list_files->push_back(*master_repository_location() / "profiles" / "arch.list");
        _imp->repository_mask_files->push_back(*master_repository_location() / "profiles" / "package.mask");
        _imp->profiles_desc_files->push_back(*master_repository_location() / "profiles" / "profiles.desc");
        _imp->mirror_files->push_back(*master_repository_location() / "profiles" / "thirdpartymirrors");
        _imp->use_desc_dirs->push_back(*master_repository_location() / "profiles");
    }

    _imp->arch_list_files->push_back(_imp->tree_root / "profiles" / "arch.list");
    _imp->repository_mask_files->push_back(_imp->tree_root / "profiles" / "package.mask");
    _imp->profiles_desc_files->push_back(_imp->tree_root / "profiles" / "profiles.desc");
    _imp->mirror_files->push_back(_imp->tree_root / "profiles" / "thirdpartymirrors");
    _imp->use_desc_dirs->push_back(_imp->tree_root / "profiles");
}

TraditionalLayout::~TraditionalLayout()
{
}

void
TraditionalLayout::need_category_names() const
{
    Lock l(_imp->big_nasty_mutex);

    if (_imp->has_category_names)
        return;

    Context context("When loading category names for " + stringify(_imp->repository->name()) + ":");

    Log::get_instance()->message(ll_debug, lc_context, "need_category_names");

    bool found_one(false);

    std::list<FSEntry> cats_list;
    if (master_repository_location())
        cats_list.push_back(*master_repository_location() / "profiles" / "categories");
    cats_list.push_back(_imp->tree_root / "profiles" / "categories");

    for (std::list<FSEntry>::const_iterator i(cats_list.begin()), i_end(cats_list.end()) ;
            i != i_end ; ++i)
    {
        if (! i->exists())
            continue;

        LineConfigFile cats(*i, LineConfigFileOptions());

        for (LineConfigFile::Iterator line(cats.begin()), line_end(cats.end()) ;
                line != line_end ; ++line)
        {
            try
            {
                _imp->category_names.insert(std::make_pair(CategoryNamePart(*line), false));
            }
            catch (const NameError & e)
            {
                Log::get_instance()->message(ll_warning, lc_context, "Skipping line '"
                        + *line + "' in '" + stringify(*i) + "' due to exception '"
                        + stringify(e.message()) + "' ('" + e.what() + ")");
            }
        }

        found_one = true;
    }

    if (! found_one)
    {
        Log::get_instance()->message(ll_qa, lc_context, "No categories file for repository at '"
                + stringify(_imp->tree_root) + "', faking it");
        for (DirIterator d(_imp->tree_root), d_end ; d != d_end ; ++d)
        {
            if (! d->is_directory_or_symlink_to_directory())
                continue;

            std::string n(d->basename());
            if (n == "CVS" || n == "distfiles" || n == "scripts" || n == "eclass" || n == "licences"
                    || n == "packages")
                continue;

            try
            {
                _imp->category_names.insert(std::make_pair(CategoryNamePart(n), false));
            }
            catch (const NameError &)
            {
            }
        }
    }

    _imp->has_category_names = true;
}

void
TraditionalLayout::need_package_ids(const QualifiedPackageName & n) const
{
    Lock l(_imp->big_nasty_mutex);

    using namespace tr1::placeholders;

    if (_imp->package_names[n])
        return;

    Context context("When loading versions for '" + stringify(n) + "' in "
            + stringify(_imp->repository->name()) + ":");

    tr1::shared_ptr<PackageIDSequence> v(new PackageIDSequence);

    FSEntry path(_imp->tree_root / stringify(n.category) / stringify(n.package));

    for (DirIterator e(path), e_end ; e != e_end ; ++e)
    {
        if (! _imp->entries->is_package_file(n, *e))
            continue;

        try
        {
            tr1::shared_ptr<const PackageID> id(_imp->entries->make_id(n, _imp->entries->extract_package_file_version(n, *e), *e, ""));
            if (v->end() != std::find_if(v->begin(), v->end(),
                        tr1::bind(std::equal_to<VersionSpec>(), id->version(), tr1::bind(tr1::mem_fn(&PackageID::version), _1))))
                Log::get_instance()->message(ll_warning, lc_context, "Ignoring entry '" + stringify(*e)
                        + "' for '" + stringify(n) + "' in repository '" + stringify(_imp->repository->name())
                        + "' because another equivalent version already exists");
            v->push_back(id);
        }
        catch (const Exception & ee)
        {
            Log::get_instance()->message(ll_warning, lc_context, "Skipping entry '"
                    + stringify(*e) + "' for '" + stringify(n) + "' in repository '"
                    + stringify(_imp->repository->name()) + "' due to exception '" + ee.message() + "' ("
                    + ee.what() + ")'");
        }
    }

    _imp->ids.insert(std::make_pair(n, v));
    _imp->package_names[n] = true;
}

bool
TraditionalLayout::has_category_named(const CategoryNamePart & c) const
{
    Lock l(_imp->big_nasty_mutex);

    Context context("When checking for category '" + stringify(c) + "' in '" + stringify(_imp->repository->name()) + "':");

    need_category_names();
    return _imp->category_names.end() != _imp->category_names.find(c);
}

bool
TraditionalLayout::has_package_named(const QualifiedPackageName & q) const
{
    Lock l(_imp->big_nasty_mutex);

    Context context("When checking for package '" + stringify(q) + "' in '" + stringify(_imp->repository->name()) + ":");

    need_category_names();

    CategoryMap::iterator cat_iter(_imp->category_names.find(q.category));

    if (_imp->category_names.end() == cat_iter)
        return false;

    if (cat_iter->second)
    {
        /* this category's package names are fully loaded */
        return _imp->package_names.find(q) != _imp->package_names.end();
    }
    else
    {
        /* package names are only partially loaded or not loaded */
        if (_imp->package_names.find(q) != _imp->package_names.end())
            return true;

        FSEntry fs(_imp->tree_root);
        fs /= stringify(q.category);
        fs /= stringify(q.package);
        if (! fs.is_directory_or_symlink_to_directory())
            return false;
        _imp->package_names.insert(std::make_pair(q, false));
        return true;
    }
}

void
TraditionalLayout::need_category_names_collection() const
{
    Lock l(_imp->big_nasty_mutex);

    if (_imp->category_names_collection)
        return;

    need_category_names();

    _imp->category_names_collection.reset(new CategoryNamePartSet);
    std::copy(_imp->category_names.begin(), _imp->category_names.end(),
            transform_inserter(_imp->category_names_collection->inserter(),
                tr1::mem_fn(&std::pair<const CategoryNamePart, bool>::first)));
}

tr1::shared_ptr<const CategoryNamePartSet>
TraditionalLayout::category_names() const
{
    Lock l(_imp->big_nasty_mutex);

    Context context("When fetching category names in " + stringify(stringify(_imp->repository->name())) + ":");

    need_category_names_collection();
    return _imp->category_names_collection;
}

tr1::shared_ptr<const QualifiedPackageNameSet>
TraditionalLayout::package_names(const CategoryNamePart & c) const
{
    Lock l(_imp->big_nasty_mutex);

    using namespace tr1::placeholders;

    /* this isn't particularly fast because it isn't called very often. avoid
     * changing the data structures used to make this faster at the expense of
     * slowing down single item queries. */

    Context context("When fetching package names in category '" + stringify(c)
            + "' in " + stringify(_imp->repository->name()) + ":");

    need_category_names();

    if (_imp->category_names.end() == _imp->category_names.find(c))
        return tr1::shared_ptr<QualifiedPackageNameSet>(new QualifiedPackageNameSet);

    if ((_imp->tree_root / stringify(c)).is_directory_or_symlink_to_directory())
        for (DirIterator d(_imp->tree_root / stringify(c)), d_end ; d != d_end ; ++d)
        {
            try
            {
                if (! d->is_directory_or_symlink_to_directory())
                    continue;

                if (DirIterator() == std::find_if(DirIterator(*d), DirIterator(),
                            tr1::bind(&ERepositoryEntries::is_package_file, _imp->entries.get(),
                                c + PackageNamePart(d->basename()), _1)))
                    continue;

                _imp->package_names.insert(std::make_pair(c + PackageNamePart(d->basename()), false));
            }
            catch (const NameError & e)
            {
                Log::get_instance()->message(ll_warning, lc_context, "Skipping entry '" +
                        d->basename() + "' in category '" + stringify(c) + "' in repository '"
                        + stringify(_imp->repository->name()) + "' (" + e.message() + ")");
            }
        }

    _imp->category_names[c] = true;

    tr1::shared_ptr<QualifiedPackageNameSet> result(new QualifiedPackageNameSet);

    for (PackagesMap::const_iterator p(_imp->package_names.begin()), p_end(_imp->package_names.end()) ;
            p != p_end ; ++p)
        if (p->first.category == c)
            result->insert(p->first);

    return result;
}

tr1::shared_ptr<const PackageIDSequence>
TraditionalLayout::package_ids(const QualifiedPackageName & n) const
{
    Lock l(_imp->big_nasty_mutex);

    Context context("When fetching versions of '" + stringify(n) + "' in " + stringify(_imp->repository->name()) + ":");

    if (has_package_named(n))
    {
        need_package_ids(n);
        return _imp->ids.find(n)->second;
    }
    else
        return tr1::shared_ptr<PackageIDSequence>(new PackageIDSequence);
}

FSEntry
TraditionalLayout::info_packages_file(const FSEntry & dir) const
{
    return dir / "info_pkgs";
}

FSEntry
TraditionalLayout::info_variables_file(const FSEntry & dir) const
{
    return dir / "info_vars";
}

FSEntry
TraditionalLayout::package_directory(const QualifiedPackageName & qpn) const
{
    return _imp->tree_root / stringify(qpn.category) / stringify(qpn.package);
}

FSEntry
TraditionalLayout::category_directory(const CategoryNamePart & cat) const
{
    return _imp->tree_root / stringify(cat);
}

tr1::shared_ptr<const FSEntrySequence>
TraditionalLayout::arch_list_files() const
{
    return _imp->arch_list_files;
}

tr1::shared_ptr<const FSEntrySequence>
TraditionalLayout::repository_mask_files() const
{
    return _imp->repository_mask_files;
}

tr1::shared_ptr<const FSEntrySequence>
TraditionalLayout::profiles_desc_files() const
{
    return _imp->profiles_desc_files;
}

tr1::shared_ptr<const FSEntrySequence>
TraditionalLayout::mirror_files() const
{
    return _imp->mirror_files;
}

tr1::shared_ptr<const FSEntrySequence>
TraditionalLayout::use_desc_dirs() const
{
    return _imp->use_desc_dirs;
}

bool
TraditionalLayout::eapi_ebuild_suffix() const
{
    return false;
}

FSEntry
TraditionalLayout::package_file(const PackageID & id) const
{
    for (DirIterator d(package_directory(id.name())), d_end ; d != d_end ; ++d)
    {
        std::string::size_type p(d->basename().rfind('.'));
        if (std::string::npos == p)
            continue;

        std::string prefix(stringify(id.name().package) + "-" + stringify(id.version()));
        if (0 == d->basename().compare(0, p, prefix))
            if (_imp->entries->is_package_file(id.name(), *d))
                return *d;
    }

    throw NoSuchPackageError(stringify(id));
}

FSEntry
TraditionalLayout::profiles_base_dir() const
{
    return _imp->tree_root / "profiles";
}

tr1::shared_ptr<const FSEntrySequence>
TraditionalLayout::exlibsdirs(const QualifiedPackageName & q) const
{
    tr1::shared_ptr<FSEntrySequence> result(new FSEntrySequence);

    if (_imp->repository->params().master_repository)
        result->push_back(_imp->repository->params().master_repository->params().location / "exlibs");
    result->push_back(_imp->tree_root / "exlibs");
    if (_imp->repository->params().master_repository)
        result->push_back(_imp->repository->params().master_repository->layout()->category_directory(q.category) / "exlibs");
    result->push_back(_imp->repository->layout()->category_directory(q.category) / "exlibs");
    if (_imp->repository->params().master_repository)
        result->push_back(_imp->repository->params().master_repository->layout()->package_directory(q));
    result->push_back(_imp->repository->layout()->package_directory(q));

    return result;
}

