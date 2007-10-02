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

#include <test/test_runner.hh>
#include <test/test_framework.hh>
#include <paludis/repositories/gems/gem_specification.hh>
#include <paludis/repositories/gems/yaml.hh>
#include <paludis/environments/test/test_environment.hh>
#include <paludis/util/visitor-impl.hh>
#include <paludis/name.hh>
#include <paludis/metadata_key.hh>
#include <paludis/version_spec.hh>
#include <libwrapiter/libwrapiter_forward_iterator.hh>

using namespace test;
using namespace paludis;
using namespace paludis::gems;

namespace
{
    struct ExtractValueVisitor :
        ConstVisitor<MetadataKeyVisitorTypes>
    {
        std::string s;

        void visit(const MetadataStringKey & k)
        {
            s = k.value();
        }

        void visit(const MetadataPackageIDKey &)
        {
        }

        void visit(const MetadataSetKey<KeywordNameSet> &)
        {
        }

        void visit(const MetadataSetKey<UseFlagNameSet> &)
        {
        }

        void visit(const MetadataSetKey<IUseFlagSet> &)
        {
        }

        void visit(const MetadataSetKey<Set<std::string> > &)
        {
        }

        void visit(const MetadataSetKey<PackageIDSequence> &)
        {
        }

        void visit(const MetadataSpecTreeKey<DependencySpecTree> &)
        {
        }

        void visit(const MetadataSpecTreeKey<LicenseSpecTree> &)
        {
        }

        void visit(const MetadataSpecTreeKey<ProvideSpecTree> &)
        {
        }

        void visit(const MetadataSpecTreeKey<RestrictSpecTree> &)
        {
        }

        void visit(const MetadataSpecTreeKey<FetchableURISpecTree> &)
        {
        }

        void visit(const MetadataSpecTreeKey<SimpleURISpecTree> &)
        {
        }

        void visit(const MetadataContentsKey &)
        {
        }

        void visit(const MetadataTimeKey &)
        {
        }

        void visit(const MetadataRepositoryMaskInfoKey &)
        {
        }

        void visit(const MetadataFSEntryKey &)
        {
        }
    };
}

namespace test_cases
{
    struct SpecificationTest : TestCase
    {
        SpecificationTest() : TestCase("gem specification") { }

        void run()
        {
            std::string spec_text(
                "--- !ruby/object:Gem::Specification\n"
                "name: demo\n"
                "version: !ruby/object:Gem::Version\n"
                "  version: 1.2.3\n"
                "summary: This is the summary\n"
                "homepage:\n"
                "rubyforge_project:\n"
                "description: A longer description\n"
                "platform: ruby\n"
                "date: 1234\n"
                "authors: [ Fred , Barney ]\n"
                );

            yaml::Document spec_doc(spec_text);
            TEST_CHECK(spec_doc.top());

            TestEnvironment env;
            GemSpecification spec(&env, tr1::shared_ptr<Repository>(), *spec_doc.top());

            TEST_CHECK(spec.short_description_key());
            TEST_CHECK_EQUAL(spec.short_description_key()->value(), "This is the summary");
            TEST_CHECK_EQUAL(spec.name(), QualifiedPackageName("gems/demo"));
            TEST_CHECK_EQUAL(spec.version(), VersionSpec("1.2.3"));
            TEST_CHECK(spec.find_metadata("rubyforge_project") == spec.end_metadata());
            TEST_CHECK(spec.long_description_key());
            TEST_CHECK_EQUAL(spec.long_description_key()->value(), "A longer description");

            TEST_CHECK(spec.find_metadata("authors") != spec.end_metadata());
            ExtractValueVisitor v;
            (*spec.find_metadata("authors"))->accept(v);
            TEST_CHECK_EQUAL(v.s, "Fred, Barney");

#if 0
            TEST_CHECK_EQUAL(spec.homepage(), "");
            TEST_CHECK_EQUAL(spec.rubyforge_project(), "");
            TEST_CHECK_EQUAL(spec.date(), "1234");
#endif
        }
    } test_specification;
}

