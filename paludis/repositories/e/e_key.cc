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

#include <paludis/repositories/e/e_key.hh>
#include <paludis/repositories/e/ebuild_id.hh>
#include <paludis/repositories/e/dep_parser.hh>
#include <paludis/repositories/e/eapi.hh>
#include <paludis/repositories/e/dep_spec_pretty_printer.hh>

#include <paludis/util/private_implementation_pattern-impl.hh>
#include <paludis/util/stringify.hh>
#include <paludis/util/tokeniser.hh>
#include <paludis/util/iterator.hh>
#include <paludis/util/fs_entry.hh>
#include <paludis/util/log.hh>
#include <paludis/util/mutex.hh>
#include <paludis/util/set.hh>
#include <paludis/util/tr1_functional.hh>
#include <paludis/util/idle_action_pool.hh>
#include <paludis/util/join.hh>
#include <paludis/util/visitor-impl.hh>

#include <paludis/contents.hh>
#include <paludis/repository.hh>
#include <paludis/environment.hh>
#include <paludis/stringify_formatter-impl.hh>
#include <paludis/dep_spec_flattener.hh>

#include <libwrapiter/libwrapiter_forward_iterator.hh>
#include <libwrapiter/libwrapiter_output_iterator.hh>

#include <list>
#include <vector>
#include <fstream>
#include <map>

using namespace paludis;
using namespace paludis::erepository;

EStringKey::EStringKey(const tr1::shared_ptr<const ERepositoryID> &,
        const std::string & r, const std::string & h, const std::string & v, const MetadataKeyType t) :
    MetadataStringKey(r, h, t),
    _value(v)
{
}

EStringKey::~EStringKey()
{
}

const std::string
EStringKey::value() const
{
    return _value;
}

EMutableRepositoryMaskInfoKey::EMutableRepositoryMaskInfoKey(const tr1::shared_ptr<const ERepositoryID> &,
        const std::string & r, const std::string & h, tr1::shared_ptr<const RepositoryMaskInfo> v, const MetadataKeyType t) :
    MetadataRepositoryMaskInfoKey(r, h, t),
    _value(v)
{
}

EMutableRepositoryMaskInfoKey::~EMutableRepositoryMaskInfoKey()
{
}

const tr1::shared_ptr<const RepositoryMaskInfo>
EMutableRepositoryMaskInfoKey::value() const
{
    return _value;
}

void
EMutableRepositoryMaskInfoKey::set_value(tr1::shared_ptr<const RepositoryMaskInfo> v)
{
    _value = v;
}

namespace paludis
{
    template <>
    struct Implementation<EDependenciesKey>
    {
        const Environment * const env;
        const tr1::shared_ptr<const ERepositoryID> id;
        const std::string string_value;
        mutable Mutex value_mutex;
        mutable tr1::shared_ptr<const DependencySpecTree::ConstItem> value;
        mutable tr1::function<void () throw ()> value_used;

        Implementation(
                const Environment * const e,
                const tr1::shared_ptr<const ERepositoryID> & i, const std::string & v) :
            env(e),
            id(i),
            string_value(v)
        {
        }
    };
}

EDependenciesKey::EDependenciesKey(
        const Environment * const e,
        const tr1::shared_ptr<const ERepositoryID> & id,
        const std::string & r, const std::string & h, const std::string & v, const MetadataKeyType t) :
    MetadataSpecTreeKey<DependencySpecTree>(r, h, t),
    PrivateImplementationPattern<EDependenciesKey>(new Implementation<EDependenciesKey>(e, id, v)),
    _imp(PrivateImplementationPattern<EDependenciesKey>::_imp.get())
{
}

EDependenciesKey::~EDependenciesKey()
{
}

const tr1::shared_ptr<const DependencySpecTree::ConstItem>
EDependenciesKey::value() const
{
    Lock l(_imp->value_mutex);
    if (_imp->value)
    {
        if (_imp->value_used)
        {
            _imp->value_used();
            _imp->value_used = tr1::function<void () throw ()>();
        }
        return _imp->value;
    }

    IdleActionPool::get_instance()->increase_unprepared_stat();

    Context context("When parsing metadata key '" + raw_name() + "' from '" + stringify(*_imp->id) + "':");
    _imp->value = parse_depend(_imp->string_value, *_imp->id->eapi());
    return _imp->value;
}

std::string
EDependenciesKey::pretty_print(const DependencySpecTree::Formatter & f) const
{
    StringifyFormatter ff(f);
    DepSpecPrettyPrinter p(_imp->env, _imp->id, ff, 0, true);
    value()->accept(p);
    return stringify(p);
}

std::string
EDependenciesKey::pretty_print_flat(const DependencySpecTree::Formatter & f) const
{
    StringifyFormatter ff(f);
    DepSpecPrettyPrinter p(_imp->env, _imp->id, ff, 0, false);
    value()->accept(p);
    return stringify(p);
}

IdleActionResult
EDependenciesKey::idle_load() const
{
    TryLock l(_imp->value_mutex);
    if (l() && ! _imp->value)
    {
        try
        {
            Context context("When parsing metadata key '" + raw_name() + "' from '" + stringify(*_imp->id) + "' as idle action:");
            _imp->value = parse_depend(_imp->string_value, *_imp->id->eapi());
            _imp->value_used = tr1::bind(tr1::mem_fn(&IdleActionPool::increase_used_stat), IdleActionPool::get_instance());
            return iar_success;
        }
        catch (...)
        {
            // the exception will be repeated in the relevant thread
            return iar_failure;
        }
    }

    return iar_already_completed;
}

namespace paludis
{
    template <>
    struct Implementation<ELicenseKey>
    {
        const Environment * const env;
        const tr1::shared_ptr<const ERepositoryID> id;
        const std::string string_value;
        mutable Mutex value_mutex;
        mutable tr1::shared_ptr<const LicenseSpecTree::ConstItem> value;
        mutable tr1::function<void () throw ()> value_used;

        Implementation(const Environment * const e,
                const tr1::shared_ptr<const ERepositoryID> & i, const std::string & v) :
            env(e),
            id(i),
            string_value(v)
        {
        }
    };
}

ELicenseKey::ELicenseKey(
        const Environment * const e,
        const tr1::shared_ptr<const ERepositoryID> & id,
        const std::string & r, const std::string & h, const std::string & v, const MetadataKeyType t) :
    MetadataSpecTreeKey<LicenseSpecTree>(r, h, t),
    PrivateImplementationPattern<ELicenseKey>(new Implementation<ELicenseKey>(e, id, v)),
    _imp(PrivateImplementationPattern<ELicenseKey>::_imp.get())
{
}

ELicenseKey::~ELicenseKey()
{
}

const tr1::shared_ptr<const LicenseSpecTree::ConstItem>
ELicenseKey::value() const
{
    Lock l(_imp->value_mutex);
    if (_imp->value)
    {
        if (_imp->value_used)
        {
            _imp->value_used();
            _imp->value_used = tr1::function<void () throw ()>();
        }
        return _imp->value;
    }

    IdleActionPool::get_instance()->increase_unprepared_stat();

    Context context("When parsing metadata key '" + raw_name() + "' from '" + stringify(*_imp->id) + "':");
    _imp->value = parse_license(_imp->string_value, *_imp->id->eapi());
    return _imp->value;
}

std::string
ELicenseKey::pretty_print(const LicenseSpecTree::Formatter & f) const
{
    StringifyFormatter ff(f);
    DepSpecPrettyPrinter p(_imp->env, _imp->id, ff, 0, true);
    value()->accept(p);
    return stringify(p);
}

std::string
ELicenseKey::pretty_print_flat(const LicenseSpecTree::Formatter & f) const
{
    StringifyFormatter ff(f);
    DepSpecPrettyPrinter p(_imp->env, _imp->id, ff, 0, false);
    value()->accept(p);
    return stringify(p);
}

IdleActionResult
ELicenseKey::idle_load() const
{
    TryLock l(_imp->value_mutex);
    if (l() && ! _imp->value)
    {
        try
        {
            Context context("When parsing metadata key '" + raw_name() + "' from '" + stringify(*_imp->id) + "' as idle action:");
            _imp->value = parse_license(_imp->string_value, *_imp->id->eapi());
            _imp->value_used = tr1::bind(tr1::mem_fn(&IdleActionPool::increase_used_stat), IdleActionPool::get_instance());
            return iar_success;
        }
        catch (...)
        {
            // the exception will be repeated in the relevant thread
            return iar_failure;
        }
    }

    return iar_already_completed;
}

namespace paludis
{
    template <>
    struct Implementation<EFetchableURIKey>
    {
        const Environment * const env;
        const tr1::shared_ptr<const ERepositoryID> id;
        const std::string string_value;
        mutable Mutex value_mutex;
        mutable tr1::shared_ptr<const FetchableURISpecTree::ConstItem> value;
        mutable tr1::shared_ptr<const URILabel> initial_label;

        Implementation(const Environment * const e, const tr1::shared_ptr<const ERepositoryID> & i, const std::string & v) :
            env(e),
            id(i),
            string_value(v)
        {
        }
    };
}

EFetchableURIKey::EFetchableURIKey(const Environment * const e,
        const tr1::shared_ptr<const ERepositoryID> & id,
        const std::string & r, const std::string & h, const std::string & v, const MetadataKeyType t) :
    MetadataSpecTreeKey<FetchableURISpecTree>(r, h, t),
    PrivateImplementationPattern<EFetchableURIKey>(new Implementation<EFetchableURIKey>(e, id, v)),
    _imp(PrivateImplementationPattern<EFetchableURIKey>::_imp.get())
{
}

EFetchableURIKey::~EFetchableURIKey()
{
}

const tr1::shared_ptr<const FetchableURISpecTree::ConstItem>
EFetchableURIKey::value() const
{
    Lock l(_imp->value_mutex);

    if (_imp->value)
        return _imp->value;

    Context context("When parsing metadata key '" + raw_name() + "' from '" + stringify(*_imp->id) + "':");
    _imp->value = parse_fetchable_uri(_imp->string_value, *_imp->id->eapi());
    return _imp->value;
}

std::string
EFetchableURIKey::pretty_print(const FetchableURISpecTree::Formatter & f) const
{
    StringifyFormatter ff(f);
    DepSpecPrettyPrinter p(_imp->env, _imp->id, ff, 0, true);
    value()->accept(p);
    return stringify(p);
}

std::string
EFetchableURIKey::pretty_print_flat(const FetchableURISpecTree::Formatter & f) const
{
    StringifyFormatter ff(f);
    DepSpecPrettyPrinter p(_imp->env, _imp->id, ff, 0, false);
    value()->accept(p);
    return stringify(p);
}

const tr1::shared_ptr<const URILabel>
EFetchableURIKey::initial_label() const
{
    Lock l(_imp->value_mutex);

    if (! _imp->initial_label)
    {
        DepSpecFlattener f(_imp->env, _imp->id);
        if (_imp->id->restrict_key())
            _imp->id->restrict_key()->value()->accept(f);
        for (DepSpecFlattener::ConstIterator i(f.begin()), i_end(f.end()) ;
                i != i_end ; ++i)
        {
            if (_imp->id->eapi()->supported->ebuild_options->restrict_fetch->end() !=
                    std::find(_imp->id->eapi()->supported->ebuild_options->restrict_fetch->begin(),
                        _imp->id->eapi()->supported->ebuild_options->restrict_fetch->end(), (*i)->text()))
                _imp->initial_label = *parse_uri_label("default-restrict-fetch:", *_imp->id->eapi())->begin();

            else if (_imp->id->eapi()->supported->ebuild_options->restrict_fetch->end() !=
                    std::find(_imp->id->eapi()->supported->ebuild_options->restrict_fetch->begin(),
                        _imp->id->eapi()->supported->ebuild_options->restrict_fetch->end(), (*i)->text()))
                _imp->initial_label = *parse_uri_label("default-restrict-mirror:", *_imp->id->eapi())->begin();
        }

        if (! _imp->initial_label)
            _imp->initial_label = *parse_uri_label("default:", *_imp->id->eapi())->begin();
    }

    return _imp->initial_label;
}

namespace paludis
{
    template <>
    struct Implementation<ESimpleURIKey>
    {
        const Environment * const env;
        const tr1::shared_ptr<const ERepositoryID> id;
        const std::string string_value;
        mutable Mutex value_mutex;
        mutable tr1::shared_ptr<const SimpleURISpecTree::ConstItem> value;

        Implementation(const Environment * const e, const tr1::shared_ptr<const ERepositoryID> & i, const std::string & v) :
            env(e),
            id(i),
            string_value(v)
        {
        }
    };
}

ESimpleURIKey::ESimpleURIKey(const Environment * const e,
        const tr1::shared_ptr<const ERepositoryID> & id,
        const std::string & r, const std::string & h, const std::string & v, const MetadataKeyType t) :
    MetadataSpecTreeKey<SimpleURISpecTree>(r, h, t),
    PrivateImplementationPattern<ESimpleURIKey>(new Implementation<ESimpleURIKey>(e, id, v)),
    _imp(PrivateImplementationPattern<ESimpleURIKey>::_imp.get())
{
}

ESimpleURIKey::~ESimpleURIKey()
{
}

const tr1::shared_ptr<const SimpleURISpecTree::ConstItem>
ESimpleURIKey::value() const
{
    Lock l(_imp->value_mutex);

    if (_imp->value)
        return _imp->value;

    Context context("When parsing metadata key '" + raw_name() + "' from '" + stringify(*_imp->id) + "':");
    _imp->value = parse_simple_uri(_imp->string_value, *_imp->id->eapi());
    return _imp->value;
}

std::string
ESimpleURIKey::pretty_print(const SimpleURISpecTree::Formatter & f) const
{
    StringifyFormatter ff(f);
    DepSpecPrettyPrinter p(_imp->env, _imp->id, ff, 0, true);
    value()->accept(p);
    return stringify(p);
}

std::string
ESimpleURIKey::pretty_print_flat(const SimpleURISpecTree::Formatter & f) const
{
    StringifyFormatter ff(f);
    DepSpecPrettyPrinter p(_imp->env, _imp->id, ff, 0, false);
    value()->accept(p);
    return stringify(p);
}

namespace paludis
{
    template <>
    struct Implementation<ERestrictKey>
    {
        const Environment * const env;
        const tr1::shared_ptr<const ERepositoryID> id;
        const std::string string_value;
        mutable Mutex value_mutex;
        mutable tr1::shared_ptr<const RestrictSpecTree::ConstItem> value;

        Implementation(const Environment * const e, const tr1::shared_ptr<const ERepositoryID> & i, const std::string & v) :
            env(e),
            id(i),
            string_value(v)
        {
        }
    };
}

ERestrictKey::ERestrictKey(const Environment * const e,
        const tr1::shared_ptr<const ERepositoryID> & id,
        const std::string & r, const std::string & h, const std::string & v, const MetadataKeyType t) :
    MetadataSpecTreeKey<RestrictSpecTree>(r, h, t),
    PrivateImplementationPattern<ERestrictKey>(new Implementation<ERestrictKey>(e, id, v)),
    _imp(PrivateImplementationPattern<ERestrictKey>::_imp.get())
{
}

ERestrictKey::~ERestrictKey()
{
}

const tr1::shared_ptr<const RestrictSpecTree::ConstItem>
ERestrictKey::value() const
{
    Lock l(_imp->value_mutex);

    if (_imp->value)
        return _imp->value;

    Context context("When parsing metadata key '" + raw_name() + "' from '" + stringify(*_imp->id) + "':");
    _imp->value = parse_restrict(_imp->string_value, *_imp->id->eapi());
    return _imp->value;
}

std::string
ERestrictKey::pretty_print(const RestrictSpecTree::Formatter & f) const
{
    StringifyFormatter ff(f);
    DepSpecPrettyPrinter p(_imp->env, _imp->id, ff, 0, true);
    value()->accept(p);
    return stringify(p);
}

std::string
ERestrictKey::pretty_print_flat(const RestrictSpecTree::Formatter & f) const
{
    StringifyFormatter ff(f);
    DepSpecPrettyPrinter p(_imp->env, _imp->id, ff, 0, false);
    value()->accept(p);
    return stringify(p);
}

namespace paludis
{
    template <>
    struct Implementation<EProvideKey>
    {
        const Environment * const env;
        const tr1::shared_ptr<const ERepositoryID> id;
        const std::string string_value;
        mutable Mutex value_mutex;
        mutable tr1::shared_ptr<const ProvideSpecTree::ConstItem> value;

        Implementation(const Environment * const e, const tr1::shared_ptr<const ERepositoryID> & i, const std::string & v) :
            env(e),
            id(i),
            string_value(v)
        {
        }
    };
}

EProvideKey::EProvideKey(const Environment * const e, const tr1::shared_ptr<const ERepositoryID> & id,
        const std::string & r, const std::string & h, const std::string & v, const MetadataKeyType t) :
    MetadataSpecTreeKey<ProvideSpecTree>(r, h, t),
    PrivateImplementationPattern<EProvideKey>(new Implementation<EProvideKey>(e, id, v)),
    _imp(PrivateImplementationPattern<EProvideKey>::_imp.get())
{
}

EProvideKey::~EProvideKey()
{
}

const tr1::shared_ptr<const ProvideSpecTree::ConstItem>
EProvideKey::value() const
{
    Lock l(_imp->value_mutex);

    if (_imp->value)
        return _imp->value;

    Context context("When parsing metadata key '" + raw_name() + "' from '" + stringify(*_imp->id) + "':");
    _imp->value = parse_provide(_imp->string_value, *_imp->id->eapi());
    return _imp->value;
}

std::string
EProvideKey::pretty_print(const ProvideSpecTree::Formatter & f) const
{
    StringifyFormatter ff(f);
    DepSpecPrettyPrinter p(_imp->env, _imp->id, ff, 0, true);
    value()->accept(p);
    return stringify(p);
}

std::string
EProvideKey::pretty_print_flat(const ProvideSpecTree::Formatter & f) const
{
    StringifyFormatter ff(f);
    DepSpecPrettyPrinter p(_imp->env, _imp->id, ff, 0, false);
    value()->accept(p);
    return stringify(p);
}

namespace paludis
{
    template <>
    struct Implementation<EIUseKey>
    {
        const tr1::shared_ptr<const ERepositoryID> id;
        const Environment * const env;
        const std::string string_value;
        mutable Mutex value_mutex;
        mutable tr1::shared_ptr<IUseFlagSet> value;
        mutable tr1::function<void () throw ()> value_used;

        Implementation(const tr1::shared_ptr<const ERepositoryID> & i, const Environment * const e, const std::string & v) :
            id(i),
            env(e),
            string_value(v)
        {
        }
    };
}

EIUseKey::EIUseKey(
        const Environment * const e,
        const tr1::shared_ptr<const ERepositoryID> & id,
        const std::string & r, const std::string & h, const std::string & v, const MetadataKeyType t) :
    MetadataSetKey<IUseFlagSet>(r, h, t),
    PrivateImplementationPattern<EIUseKey>(new Implementation<EIUseKey>(id, e, v)),
    _imp(PrivateImplementationPattern<EIUseKey>::_imp.get())
{
}

EIUseKey::~EIUseKey()
{
}

const tr1::shared_ptr<const IUseFlagSet>
EIUseKey::value() const
{
    Lock l(_imp->value_mutex);
    if (_imp->value)
    {
        if (_imp->value_used)
        {
            _imp->value_used();
            _imp->value_used = tr1::function<void () throw ()>();
        }
        return _imp->value;
    }

    IdleActionPool::get_instance()->increase_unprepared_stat();

    Context context("When parsing metadata key '" + raw_name() + "' from '" + stringify(*_imp->id) + "':");
    _imp->value.reset(new IUseFlagSet);
    std::list<std::string> tokens;
    WhitespaceTokeniser::get_instance()->tokenise(_imp->string_value, std::back_inserter(tokens));

    tr1::shared_ptr<const UseFlagNameSet> prefixes;
    if (_imp->id->repository()->use_interface)
        prefixes = _imp->id->repository()->use_interface->use_expand_prefixes();
    else
        prefixes.reset(new UseFlagNameSet);

    for (std::list<std::string>::const_iterator t(tokens.begin()), t_end(tokens.end()) ;
            t != t_end ; ++t)
    {
        IUseFlag f(*t, _imp->id->eapi()->supported->iuse_flag_parse_mode, std::string::npos);
        for (UseFlagNameSet::ConstIterator p(prefixes->begin()), p_end(prefixes->end()) ;
                p != p_end ; ++p)
            if (0 == stringify(f.flag).compare(0, stringify(*p).length(), stringify(*p), 0, stringify(*p).length()))
                f.prefix_delim_pos = stringify(*p).length();
        _imp->value->insert(f);
    }

    return _imp->value;
}

IdleActionResult
EIUseKey::idle_load() const
{
    TryLock l(_imp->value_mutex);
    if (l() && ! _imp->value)
    {
        try
        {
            tr1::shared_ptr<const IUseFlagSet> PALUDIS_ATTRIBUTE((unused)) a(value());
        }
        catch (...)
        {
            // the exception will be repeated in the relevant thread
            _imp->value.reset();
            return iar_failure;
        }

        return iar_success;
    }

    return iar_already_completed;
}

std::string
EIUseKey::pretty_print_flat(const Formatter<IUseFlag> & f) const
{
    std::string result;
    std::multimap<std::string, IUseFlag> prefixes;
    for (IUseFlagSet::ConstIterator i(value()->begin()), i_end(value()->end()) ;
            i != i_end ; ++i)
    {
        if (std::string::npos != i->prefix_delim_pos)
        {
            prefixes.insert(std::make_pair(stringify(i->flag).substr(0, i->prefix_delim_pos), *i));
            continue;
        }

        if (! result.empty())
            result.append(" ");

        if (_imp->id->repository()->use_interface && _imp->id->repository()->use_interface->query_use_mask(i->flag, *_imp->id))
            result.append(f.format(*i, format::Masked()));
        else if (_imp->id->repository()->use_interface && _imp->id->repository()->use_interface->query_use_force(i->flag, *_imp->id))
            result.append(f.format(*i, format::Forced()));
        else if (_imp->env->query_use(i->flag, *_imp->id))
            result.append(f.format(*i, format::Enabled()));
        else
            result.append(f.format(*i, format::Disabled()));
    }

    for (std::multimap<std::string, IUseFlag>::const_iterator j(prefixes.begin()), j_end(prefixes.end()) ;
            j != j_end ; ++j)
    {
        if (! result.empty())
            result.append(" ");

        if (_imp->id->repository()->use_interface && _imp->id->repository()->use_interface->query_use_mask(j->second.flag, *_imp->id))
            result.append(f.format(j->second, format::Masked()));
        else if (_imp->id->repository()->use_interface && _imp->id->repository()->use_interface->query_use_force(j->second.flag, *_imp->id))
            result.append(f.format(j->second, format::Forced()));
        else if (_imp->env->query_use(j->second.flag, *_imp->id))
            result.append(f.format(j->second, format::Enabled()));
        else
            result.append(f.format(j->second, format::Disabled()));
    }

    return result;
}

std::string
EIUseKey::pretty_print_flat_with_comparison(
        const Environment * const env,
        const tr1::shared_ptr<const PackageID> & id,
        const Formatter<IUseFlag> & f) const
{
    std::string result;
    std::multimap<std::string, IUseFlag> prefixes;
    for (IUseFlagSet::ConstIterator i(value()->begin()), i_end(value()->end()) ;
            i != i_end ; ++i)
    {
        if (std::string::npos != i->prefix_delim_pos)
        {
            prefixes.insert(std::make_pair(stringify(i->flag).substr(0, i->prefix_delim_pos), *i));
            continue;
        }

        if (! result.empty())
            result.append(" ");

        std::string l;
        bool n;

        if (_imp->id->repository()->use_interface && _imp->id->repository()->use_interface->query_use_mask(i->flag, *_imp->id))
        {
            l = f.format(*i, format::Masked());
            n = false;
        }
        else if (_imp->id->repository()->use_interface && _imp->id->repository()->use_interface->query_use_force(i->flag, *_imp->id))
        {
            l = f.format(*i, format::Forced());
            n = true;
        }
        else if (_imp->env->query_use(i->flag, *_imp->id))
        {
            l = f.format(*i, format::Enabled());
            n = true;
        }
        else
        {
            l = f.format(*i, format::Disabled());
            n = false;
        }

        if (! id->iuse_key())
            l = f.decorate(*i, l, format::Added());
        else
        {
            using namespace tr1::placeholders;
            IUseFlagSet::ConstIterator p(std::find_if(id->iuse_key()->value()->begin(), id->iuse_key()->value()->end(),
                        tr1::bind(std::equal_to<UseFlagName>(), i->flag, tr1::bind<const UseFlagName>(&IUseFlag::flag, _1))));

            if (p == id->iuse_key()->value()->end())
                l = f.decorate(*i, l, format::Added());
            else if (n != env->query_use(i->flag, *id))
                l = f.decorate(*i, l, format::Changed());
        }

        result.append(l);
    }

    for (std::multimap<std::string, IUseFlag>::const_iterator j(prefixes.begin()), j_end(prefixes.end()) ;
            j != j_end ; ++j)
    {
        if (! result.empty())
            result.append(" ");

        std::string l;
        bool n;

        if (_imp->id->repository()->use_interface && _imp->id->repository()->use_interface->query_use_mask(j->second.flag, *_imp->id))
        {
            l = f.format(j->second, format::Masked());
            n = false;
        }
        else if (_imp->id->repository()->use_interface && _imp->id->repository()->use_interface->query_use_force(j->second.flag, *_imp->id))
        {
            l = f.format(j->second, format::Forced());
            n = true;
        }
        else if (_imp->env->query_use(j->second.flag, *_imp->id))
        {
            l = f.format(j->second, format::Enabled());
            n = true;
        }
        else
        {
            l = f.format(j->second, format::Disabled());
            n = false;
        }

        if (! id->iuse_key())
            l = f.decorate(j->second, l, format::Added());
        else
        {
            using namespace tr1::placeholders;
            IUseFlagSet::ConstIterator p(std::find_if(id->iuse_key()->value()->begin(), id->iuse_key()->value()->end(),
                        tr1::bind(std::equal_to<UseFlagName>(), j->second.flag, tr1::bind<const UseFlagName>(&IUseFlag::flag, _1))));

            if (p == id->iuse_key()->value()->end())
                l = f.decorate(j->second, l, format::Added());
            else if (n != env->query_use(j->second.flag, *id))
                l = f.decorate(j->second, l, format::Changed());
        }

        result.append(l);
    }

    return result;
}

namespace paludis
{
    template <>
    struct Implementation<EKeywordsKey>
    {
        const tr1::shared_ptr<const ERepositoryID> id;
        const Environment * const env;
        const std::string string_value;
        mutable Mutex value_mutex;
        mutable tr1::shared_ptr<KeywordNameSet> value;
        mutable tr1::function<void () throw ()> value_used;

        Implementation(const tr1::shared_ptr<const ERepositoryID> & i, const Environment * const e, const std::string & v) :
            id(i),
            env(e),
            string_value(v)
        {
        }
    };
}

EKeywordsKey::EKeywordsKey(const Environment * const e, const tr1::shared_ptr<const ERepositoryID> & id,
        const std::string & r, const std::string & h, const std::string & v, const MetadataKeyType t) :
    MetadataSetKey<KeywordNameSet>(r, h, t),
    PrivateImplementationPattern<EKeywordsKey>(new Implementation<EKeywordsKey>(id, e, v)),
    _imp(PrivateImplementationPattern<EKeywordsKey>::_imp.get())
{
}

EKeywordsKey::~EKeywordsKey()
{
}

const tr1::shared_ptr<const KeywordNameSet>
EKeywordsKey::value() const
{
    Lock l(_imp->value_mutex);
    if (_imp->value)
    {
        if (_imp->value_used)
        {
            _imp->value_used();
            _imp->value_used = tr1::function<void () throw ()>();
        }
        return _imp->value;
    }

    IdleActionPool::get_instance()->increase_unprepared_stat();

    _imp->value.reset(new KeywordNameSet);
    Context context("When parsing metadata key '" + raw_name() + "' from '" + stringify(*_imp->id) + "':");
    WhitespaceTokeniser::get_instance()->tokenise(_imp->string_value, create_inserter<KeywordName>(_imp->value->inserter()));
    return _imp->value;
}

IdleActionResult
EKeywordsKey::idle_load() const
{
    TryLock l(_imp->value_mutex);
    if (l() && ! _imp->value)
    {
        try
        {
            _imp->value.reset(new KeywordNameSet);
            Context context("When parsing metadata key '" + raw_name() + "' from '" + stringify(*_imp->id) + "' as idle action:");
            WhitespaceTokeniser::get_instance()->tokenise(_imp->string_value, create_inserter<KeywordName>(_imp->value->inserter()));
            _imp->value_used = tr1::bind(tr1::mem_fn(&IdleActionPool::increase_used_stat), IdleActionPool::get_instance());
            return iar_success;
        }
        catch (...)
        {
            // the exception will be repeated in the relevant thread
            _imp->value.reset();
            return iar_failure;
        }
    }

    return iar_already_completed;
}

std::string
EKeywordsKey::pretty_print_flat(const Formatter<KeywordName> & f) const
{
    std::string result;
    for (KeywordNameSet::ConstIterator i(value()->begin()), i_end(value()->end()) ;
            i != i_end ; ++i)
    {
        if (! result.empty())
            result.append(" ");

        tr1::shared_ptr<KeywordNameSet> k(new KeywordNameSet);
        k->insert(*i);
        if (_imp->env->accept_keywords(k, *_imp->id))
            result.append(f.format(*i, format::Accepted()));
        else
            result.append(f.format(*i, format::Unaccepted()));
    }

    return result;
}

namespace paludis
{
    template <>
    struct Implementation<EUseKey>
    {
        const tr1::shared_ptr<const ERepositoryID> id;
        const Environment * const env;
        const std::string string_value;
        mutable Mutex value_mutex;
        mutable tr1::shared_ptr<UseFlagNameSet> value;

        Implementation(const tr1::shared_ptr<const ERepositoryID> & i, const Environment * const e, const std::string & v) :
            id(i),
            env(e),
            string_value(v)
        {
        }
    };
}

EUseKey::EUseKey(const Environment * const e, const tr1::shared_ptr<const ERepositoryID> & id,
        const std::string & r, const std::string & h, const std::string & v, const MetadataKeyType t) :
    MetadataSetKey<UseFlagNameSet>(r, h, t),
    PrivateImplementationPattern<EUseKey>(new Implementation<EUseKey>(id, e, v)),
    _imp(PrivateImplementationPattern<EUseKey>::_imp.get())
{
}

EUseKey::~EUseKey()
{
}

const tr1::shared_ptr<const UseFlagNameSet>
EUseKey::value() const
{
    Lock l(_imp->value_mutex);

    if (_imp->value)
        return _imp->value;

    _imp->value.reset(new UseFlagNameSet);
    Context context("When parsing metadata key '" + raw_name() + "' from '" + stringify(*_imp->id) + "':");
    std::list<std::string> tokens;
    WhitespaceTokeniser::get_instance()->tokenise(_imp->string_value, std::back_inserter(tokens));
    for (std::list<std::string>::const_iterator t(tokens.begin()), t_end(tokens.end()) ;
            t != t_end ; ++t)
        if ('-' != t->at(0))
            _imp->value->insert(UseFlagName(*t));
    return _imp->value;
}

std::string
EUseKey::pretty_print_flat(const Formatter<UseFlagName> & f) const
{
    std::string result;
    for (UseFlagNameSet::ConstIterator i(value()->begin()), i_end(value()->end()) ;
            i != i_end ; ++i)
    {
        if (! result.empty())
            result.append(" ");

        if (_imp->id->repository()->use_interface && _imp->id->repository()->use_interface->query_use_mask(*i, *_imp->id))
            result.append(f.format(*i, format::Masked()));
        else if (_imp->id->repository()->use_interface && _imp->id->repository()->use_interface->query_use_force(*i, *_imp->id))
            result.append(f.format(*i, format::Forced()));
        else if (_imp->env->query_use(*i, *_imp->id))
            result.append(f.format(*i, format::Enabled()));
        else
            result.append(f.format(*i, format::Disabled()));
    }

    return result;
}

namespace paludis
{
    template <>
    struct Implementation<EInheritedKey>
    {
        const tr1::shared_ptr<const ERepositoryID> id;
        const std::string string_value;
        mutable Mutex value_mutex;
        mutable tr1::shared_ptr<Set<std::string> > value;

        Implementation(const tr1::shared_ptr<const ERepositoryID> & i, const std::string & v) :
            id(i),
            string_value(v)
        {
        }
    };
}

EInheritedKey::EInheritedKey(const tr1::shared_ptr<const ERepositoryID> & id,
        const std::string & r, const std::string & h, const std::string & v, const MetadataKeyType t) :
    MetadataSetKey<Set<std::string> >(r, h, t),
    PrivateImplementationPattern<EInheritedKey>(new Implementation<EInheritedKey>(id, v)),
    _imp(PrivateImplementationPattern<EInheritedKey>::_imp.get())
{
}

EInheritedKey::~EInheritedKey()
{
}

const tr1::shared_ptr<const Set<std::string> >
EInheritedKey::value() const
{
    Lock l(_imp->value_mutex);

    if (_imp->value)
        return _imp->value;

    _imp->value.reset(new Set<std::string>);
    Context context("When parsing metadata key '" + raw_name() + "' from '" + stringify(*_imp->id) + "':");
    WhitespaceTokeniser::get_instance()->tokenise(_imp->string_value, _imp->value->inserter());
    return _imp->value;
}

std::string
EInheritedKey::pretty_print_flat(const Formatter<std::string> &) const
{
    return join(value()->begin(), value()->end(), " ");
}

namespace paludis
{
    template <>
    struct Implementation<EContentsKey>
    {
        const tr1::shared_ptr<const ERepositoryID> id;
        const FSEntry filename;
        mutable Mutex value_mutex;
        mutable tr1::shared_ptr<Contents> value;

        Implementation(const tr1::shared_ptr<const ERepositoryID> & i, const FSEntry & v) :
            id(i),
            filename(v)
        {
        }
    };
}

EContentsKey::EContentsKey(const tr1::shared_ptr<const ERepositoryID> & id,
        const std::string & r, const std::string & h, const FSEntry & v, const MetadataKeyType t) :
    MetadataContentsKey(r, h, t),
    PrivateImplementationPattern<EContentsKey>(new Implementation<EContentsKey>(id, v)),
    _imp(PrivateImplementationPattern<EContentsKey>::_imp.get())
{
}

EContentsKey::~EContentsKey()
{
}

const tr1::shared_ptr<const Contents>
EContentsKey::value() const
{
    Lock l(_imp->value_mutex);

    if (_imp->value)
        return _imp->value;

    Context context("When creating contents for VDB key '" + stringify(*_imp->id) + "' from '" + stringify(_imp->filename) + "':");

    _imp->value.reset(new Contents);

    FSEntry f(_imp->filename);
    if (! f.is_regular_file_or_symlink_to_regular_file())
    {
        Log::get_instance()->message(ll_warning, lc_context) << "CONTENTS lookup failed for request for '" <<
                *_imp->id << "' using '" << _imp->filename << "'";
        return _imp->value;
    }

    std::ifstream ff(stringify(f).c_str());
    if (! ff)
        throw ConfigurationError("Could not read '" + stringify(f) + "'");

    std::string line;
    unsigned line_number(0);
    while (std::getline(ff, line))
    {
        ++line_number;

        std::vector<std::string> tokens;
        WhitespaceTokeniser::get_instance()->tokenise(line, std::back_inserter(tokens));
        if (tokens.empty())
            continue;

        if (tokens.size() < 2)
        {
            Log::get_instance()->message(ll_warning, lc_no_context) << "CONTENTS has broken line " <<
                line_number << ", skipping";
            continue;
        }

        if ("obj" == tokens.at(0))
            _imp->value->add(tr1::shared_ptr<ContentsEntry>(new ContentsFileEntry(tokens.at(1))));
        else if ("dir" == tokens.at(0))
            _imp->value->add(tr1::shared_ptr<ContentsEntry>(new ContentsDirEntry(tokens.at(1))));
        else if ("misc" == tokens.at(0))
            _imp->value->add(tr1::shared_ptr<ContentsEntry>(new ContentsMiscEntry(tokens.at(1))));
        else if ("fif" == tokens.at(0))
            _imp->value->add(tr1::shared_ptr<ContentsEntry>(new ContentsFifoEntry(tokens.at(1))));
        else if ("dev" == tokens.at(0))
            _imp->value->add(tr1::shared_ptr<ContentsEntry>(new ContentsDevEntry(tokens.at(1))));
        else if ("sym" == tokens.at(0))
        {
            if (tokens.size() < 4)
            {
                Log::get_instance()->message(ll_warning, lc_no_context) << "CONTENTS has broken sym line " <<
                    line_number << ", skipping";
                continue;
            }

            _imp->value->add(tr1::shared_ptr<ContentsEntry>(new ContentsSymEntry(tokens.at(1), tokens.at(3))));
        }
    }

    return _imp->value;
}

namespace paludis
{
    template <>
    struct Implementation<ECTimeKey>
    {
        const tr1::shared_ptr<const ERepositoryID> id;
        const FSEntry filename;
        mutable Mutex value_mutex;
        mutable tr1::shared_ptr<time_t> value;

        Implementation(const tr1::shared_ptr<const ERepositoryID> & i, const FSEntry & v) :
            id(i),
            filename(v)
        {
        }
    };
}

ECTimeKey::ECTimeKey(const tr1::shared_ptr<const ERepositoryID> & id,
        const std::string & r, const std::string & h, const FSEntry & v, const MetadataKeyType t) :
    MetadataTimeKey(r, h, t),
    PrivateImplementationPattern<ECTimeKey>(new Implementation<ECTimeKey>(id, v)),
    _imp(PrivateImplementationPattern<ECTimeKey>::_imp.get())
{
}

ECTimeKey::~ECTimeKey()
{
}

const time_t
ECTimeKey::value() const
{
    Lock l(_imp->value_mutex);

    if (_imp->value)
        return *_imp->value;

    _imp->value.reset(new time_t(0));

    try
    {
        *_imp->value = _imp->filename.ctime();
    }
    catch (const FSError & e)
    {
        Log::get_instance()->message(ll_warning, lc_context) << "Couldn't get ctime for '"
            << _imp->filename << "' for ID '" << *_imp->id << "' due to exception '" << e.message()
            << "' (" << e.what() << ")";
    }

    return *_imp->value;
}

EFSLocationKey::EFSLocationKey(const tr1::shared_ptr<const ERepositoryID> &,
        const std::string & r, const std::string & h, const FSEntry & v, const MetadataKeyType t) :
    MetadataFSEntryKey(r, h, t),
    _value(v)
{
}

EFSLocationKey::~EFSLocationKey()
{
}

const FSEntry
EFSLocationKey::value() const
{
    return _value;
}

