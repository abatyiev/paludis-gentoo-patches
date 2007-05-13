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

#ifndef PALUDIS_GUARD_PALUDIS_NAME_HH
#define PALUDIS_GUARD_PALUDIS_NAME_HH 1

#include <paludis/util/collection.hh>
#include <paludis/util/exception.hh>
#include <paludis/util/instantiation_policy.hh>
#include <paludis/util/sr.hh>
#include <paludis/util/validated.hh>

#include <string>
#include <iosfwd>

/** \file
 * Declarations for various Name classes.
 *
 * \ingroup grpnames
 */

namespace paludis
{
    /**
     * A PackageNamePartError is thrown if an invalid value is assigned to
     * a PackageNamePart.
     *
     * \ingroup grpnames
     * \ingroup grpexceptions
     */
    class PALUDIS_VISIBLE PackageNamePartError : public NameError
    {
        public:
            /**
             * Constructor.
             */
            PackageNamePartError(const std::string & name) throw ();
    };

    /**
     * A PackageNamePartValidator handles validation rules for the value
     * of a PackageNamePart.
     *
     * \ingroup grpnames
     */
    struct PALUDIS_VISIBLE PackageNamePartValidator :
        private InstantiationPolicy<PackageNamePartValidator, instantiation_method::NonInstantiableTag>
    {
        /**
         * If the parameter is not a valid value for a PackageNamePart,
         * throw a PackageNamePartError.
         */
        static void validate(const std::string &);
    };

    /**
     * A PackageNamePart holds a std::string that is a valid name for the
     * package part of a QualifiedPackageName.
     *
     * \ingroup grpnames
     */
    typedef Validated<std::string, PackageNamePartValidator> PackageNamePart;

    /**
     * Holds a set of PackageNamePart instances.
     *
     * \ingroup grpnames
     */
    typedef SortedCollection<PackageNamePart> PackageNamePartCollection;

    /**
     * A CategoryNamePartError is thrown if an invalid value is assigned to
     * a CategoryNamePart.
     *
     * \ingroup grpexceptions
     * \ingroup grpnames
     */
    class PALUDIS_VISIBLE CategoryNamePartError : public NameError
    {
        public:
            /**
             * Constructor.
             */
            CategoryNamePartError(const std::string & name) throw ();
    };

    /**
     * A CategoryNamePartValidator handles validation rules for the value
     * of a CategoryNamePart.
     *
     * \ingroup grpnames
     */
    struct PALUDIS_VISIBLE CategoryNamePartValidator :
        private InstantiationPolicy<CategoryNamePartValidator, instantiation_method::NonInstantiableTag>
    {
        /**
         * If the parameter is not a valid value for a CategoryNamePart,
         * throw a CategoryNamePartError.
         */
        static void validate(const std::string &);
    };

    /**
     * A CategoryNamePart holds a std::string that is a valid name for the
     * category part of a QualifiedPackageName.
     *
     * \ingroup grpnames
     */
    typedef Validated<std::string, CategoryNamePartValidator> CategoryNamePart;

    /**
     * Holds a set of CategoryNamePart instances.
     *
     * \ingroup grpnames
     */
    typedef SortedCollection<CategoryNamePart> CategoryNamePartCollection;

    /**
     * A UseFlagNameError is thrown if an invalid value is assigned to
     * a UseFlagName.
     *
     * \ingroup grpnames
     * \ingroup grpexceptions
     */
    class PALUDIS_VISIBLE UseFlagNameError : public NameError
    {
        public:
            /**
             * Constructor.
             */
            UseFlagNameError(const std::string & name) throw ();
    };

    /**
     * An IUseFlagNameError is thrown if an invalid value is assigned to
     * an IUseFlagName.
     *
     * \ingroup grpnames
     * \ingroup grpexceptions
     */
    class PALUDIS_VISIBLE IUseFlagNameError : public NameError
    {
        public:
            ///\name Basic operations
            ///\{

            IUseFlagNameError(const std::string & name, const std::string & msg) throw ();

            IUseFlagNameError(const std::string & name) throw ();

            ///\}
    };

    /**
     * A UseFlagNameValidator handles validation rules for the value of a
     * UseFlagName.
     *
     * \ingroup grpnames
     */
    struct PALUDIS_VISIBLE UseFlagNameValidator :
        private InstantiationPolicy<UseFlagNameValidator, instantiation_method::NonInstantiableTag>
    {
        /**
         * If the parameter is not a valid value for a UseFlagName,
         * throw a UseFlagNameError.
         */
        static void validate(const std::string &);
    };

    /**
     * A UseFlagName holds a std::string that is a valid name for a USE flag.
     *
     * \ingroup grpnames
     */
    typedef Validated<std::string, UseFlagNameValidator> UseFlagName;

    /**
     * A collection of UseFlagName instances.
     *
     * \ingroup grpnames
     */
    typedef SortedCollection<UseFlagName> UseFlagNameCollection;


#include <paludis/name-se.hh>
#include <paludis/name-sr.hh>

    /**
     * Output a QualifiedPackageName to a stream.
     *
     * \ingroup grpnames
     */
    std::ostream & operator<< (std::ostream &, const QualifiedPackageName &) PALUDIS_VISIBLE;

    /**
     * Output an IUseFlag to a stream.
     *
     * \ingroup grpnames
     */
    std::ostream & operator<< (std::ostream &, const IUseFlag &) PALUDIS_VISIBLE;

    /**
     * Holds a collection of QualifiedPackageName instances.
     *
     * \ingroup grpnames
     */
    typedef SortedCollection<QualifiedPackageName> QualifiedPackageNameCollection;

    /**
     * A QualifiedPackageNameError may be thrown if an invalid name is
     * assigned to a QualifiedPackageName (alternatively, the exception
     * raised may be a PackageNamePartError or a CategoryNamePartError).
     *
     * \ingroup grpnames
     * \ingroup grpexceptions
     */
    class PALUDIS_VISIBLE QualifiedPackageNameError : public NameError
    {
        public:
            /**
             * Constructor.
             */
            QualifiedPackageNameError(const std::string &) throw ();
    };

    /**
     * Convenience operator to make a QualifiedPackageName from a
     * PackageNamePart and a CategoryNamePart.
     *
     * \ingroup grpnames
     */
    inline const QualifiedPackageName
    operator+ (const CategoryNamePart & c, const PackageNamePart & p)
    {
        return QualifiedPackageName(c, p);
    }

    /**
     * A SlotNameError is thrown if an invalid value is assigned to
     * a SlotName.
     *
     * \ingroup grpnames
     * \ingroup grpexceptions
     */
    class PALUDIS_VISIBLE SlotNameError : public NameError
    {
        public:
            /**
             * Constructor.
             */
            SlotNameError(const std::string & name) throw ();
    };

    /**
     * A SlotNameValidator handles validation rules for the value of a
     * SlotName.
     *
     * \ingroup grpnames
     */
    struct PALUDIS_VISIBLE SlotNameValidator :
        private InstantiationPolicy<SlotNameValidator, instantiation_method::NonInstantiableTag>
    {
        /**
         * If the parameter is not a valid value for a SlotName,
         * throw a SlotNameError.
         */
        static void validate(const std::string &);
    };

    /**
     * A SlotName holds a std::string that is a valid name for a SLOT.
     *
     * \ingroup grpnames
     */
    typedef Validated<std::string, SlotNameValidator> SlotName;

    /**
     * A RepositoryNameError is thrown if an invalid value is assigned to
     * a RepositoryName.
     *
     * \ingroup grpexceptions
     * \ingroup grpnames
     */
    class PALUDIS_VISIBLE RepositoryNameError : public NameError
    {
        public:
            /**
             * Constructor.
             */
            RepositoryNameError(const std::string & name) throw ();
    };

    /**
     * A RepositoryNameValidator handles validation rules for the value
     * of a RepositoryName.
     *
     * \ingroup grpnames
     */
    struct PALUDIS_VISIBLE RepositoryNameValidator :
        private InstantiationPolicy<RepositoryNameValidator, instantiation_method::NonInstantiableTag>
    {
        /**
         * If the parameter is not a valid value for a RepositoryName,
         * throw a RepositoryNameError.
         */
        static void validate(const std::string &);
    };

    /**
     * A RepositoryName holds a std::string that is a valid name for a
     * Repository.
     *
     * \ingroup grpnames
     */
    typedef Validated<std::string, RepositoryNameValidator, comparison_mode::EqualityComparisonTag> RepositoryName;

    /**
     * Holds a collection of RepositoryName instances.
     *
     * \ingroup grpnames
     */
    typedef SequentialCollection<RepositoryName> RepositoryNameCollection;

    /**
     * Arbitrary useless comparator for RepositoryName.
     *
     * \ingroup grpnames
     */
    struct PALUDIS_VISIBLE RepositoryNameComparator
    {
        /**
         * Perform the comparison.
         */
        bool operator() (const RepositoryName & lhs, const RepositoryName & rhs) const
        {
            return lhs.data() < rhs.data();
        }
    };

    /**
     * A KeywordNameValidator handles validation rules for the value of a
     * KeywordName.
     *
     * \ingroup grpnames
     */
    struct PALUDIS_VISIBLE KeywordNameValidator :
        private InstantiationPolicy<KeywordNameValidator, instantiation_method::NonInstantiableTag>
    {
        /**
         * If the parameter is not a valid value for a KeywordName,
         * throw a KeywordNameError.
         */
        static void validate(const std::string &);
    };

    /**
     * A KeywordNameError is thrown if an invalid value is assigned to
     * a KeywordName.
     *
     * \ingroup grpnames
     * \ingroup grpexceptions
     */
    class PALUDIS_VISIBLE KeywordNameError : public NameError
    {
        public:
            /**
             * Constructor.
             */
            KeywordNameError(const std::string & name) throw ();
    };

    /**
     * A KeywordName holds a std::string that is a valid name for a KEYWORD.
     *
     * \ingroup grpnames
     */
    typedef Validated<std::string, KeywordNameValidator> KeywordName;

    /**
     * Holds a collection of KeywordName instances.
     *
     * \ingroup grpnames
     */
    typedef SortedCollection<KeywordName> KeywordNameCollection;

    /**
     * A SetNameValidator handles validation rules for the value of a
     * SetName.
     *
     * \ingroup grpnames
     */
    struct PALUDIS_VISIBLE SetNameValidator :
        private InstantiationPolicy<SetNameValidator, instantiation_method::NonInstantiableTag>
    {
        /**
         * If the parameter is not a valid value for a SetName,
         * throw a SetNameError.
         */
        static void validate(const std::string &);
    };

    /**
     * A SetNameError is thrown if an invalid value is assigned to
     * a SetName.
     *
     * \ingroup grpnames
     * \ingroup grpexceptions
     */
    class PALUDIS_VISIBLE SetNameError : public NameError
    {
        public:
            /**
             * Constructor.
             */
            SetNameError(const std::string & name) throw ();
    };

    /**
     * A SetName holds a std::string that is a valid name for a set.
     *
     * \ingroup grpnames
     */
    typedef Validated<std::string, SetNameValidator> SetName;

    /**
     * A collection of set names.
     *
     * \ingroup grpnames
     */
    typedef SortedCollection<SetName> SetNameCollection;

    /**
     * A collection of use flags.
     *
     * \ingroup grpnames
     */
    typedef SortedCollection<IUseFlag> IUseFlagCollection;

    /**
     * A collection of inherited packages.
     *
     * \ingroup grpnames
     */
    typedef SortedCollection<std::string> InheritedCollection;
}

#endif
