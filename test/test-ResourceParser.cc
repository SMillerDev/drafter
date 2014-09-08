//
//  test-ResourceParser.cc
//  snowcrash
//
//  Created by Zdenek Nemec on 5/4/13.
//  Copyright (c) 2013 Apiary Inc. All rights reserved.
//

#include "snowcrashtest.h"
#include "ResourceParser.h"

using namespace snowcrash;
using namespace snowcrashtest;

mdp::ByteBuffer ResourceFixture = \
"# My Resource [/resource/{id}{?limit}]\n\n"\
"Awesome description\n\n"\
"+ Resource Model (text/plain)\n\n"\
"        X.O.\n\n"\
"+ Parameters\n"\
"    + id = `1234` (optional, number, `0000`)\n\n"\
"        Lorem ipsum\n"\
"        + Values\n"\
"            + `1234`\n"\
"            + `0000`\n"\
"            + `beef`\n"\
"    + limit\n\n"\
"## My Method [GET]\n\n"\
"Method Description\n\n"\
"+ Response 200 (text/plain)\n\n"\
"        OK.";

TEST_CASE("Resource block classifier", "[resource]")
{
    mdp::MarkdownParser markdownParser;
    mdp::MarkdownNode markdownAST;
    SectionType sectionType;
    markdownParser.parse(ResourceFixture, markdownAST);

    REQUIRE(!markdownAST.children().empty());
    sectionType = SectionProcessor<Resource>::sectionType(markdownAST.children().begin());
    REQUIRE(sectionType == ResourceSectionType);

    // Nameless resource: "/resource"
    markdownAST.children().front().text = "/resource";
    sectionType = SectionProcessor<Resource>::sectionType(markdownAST.children().begin());
    REQUIRE(sectionType == ResourceSectionType);

    // Keyword "group"
    markdownAST.children().front().text = "Group A";
    sectionType = SectionProcessor<Resource>::sectionType(markdownAST.children().begin());
    REQUIRE(sectionType == UndefinedSectionType);

    // Resource Method
    markdownAST.children().front().text = "GET /resource";
    sectionType = SectionProcessor<Resource>::sectionType(markdownAST.children().begin());
    REQUIRE(sectionType == ResourceSectionType);
}

TEST_CASE("Parse resource", "[resource]")
{
    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(ResourceFixture, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.empty());

    REQUIRE(resource.node.name == "My Resource");
    REQUIRE(resource.node.uriTemplate == "/resource/{id}{?limit}");
    REQUIRE(resource.node.description == "Awesome description\n\n");
    REQUIRE(resource.node.headers.empty());

    REQUIRE(resource.node.model.name == "Resource");
    REQUIRE(resource.node.model.body == "X.O.\n");

    REQUIRE(resource.node.parameters.size() == 2);
    REQUIRE(resource.node.parameters[0].name == "id");
    REQUIRE(resource.node.parameters[0].description == "Lorem ipsum\n");
    REQUIRE(resource.node.parameters[1].name == "limit");

    REQUIRE(resource.node.actions.size() == 1);
    REQUIRE(resource.node.actions.front().method == "GET");
}

TEST_CASE("Parse partially defined resource", "[resource]")
{
    mdp::ByteBuffer source = \
    "# /1\n"\
    "## GET\n"\
    "+ Request\n"\
    "p1\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.size() == 2); // no response & preformatted asset
    REQUIRE(resource.report.warnings[0].code == IndentationWarning);
    REQUIRE(resource.report.warnings[1].code == EmptyDefinitionWarning);

    REQUIRE(resource.node.name.empty());
    REQUIRE(resource.node.uriTemplate == "/1");
    REQUIRE(resource.node.description.empty());
    REQUIRE(resource.node.model.name.empty());
    REQUIRE(resource.node.model.body.empty());
    REQUIRE(resource.node.actions.size() == 1);
    REQUIRE(resource.node.actions.front().method == "GET");
    REQUIRE(resource.node.actions.front().description.empty());
    REQUIRE(!resource.node.actions.front().examples.empty());
    REQUIRE(resource.node.actions.front().examples.front().requests.size() == 1);
    REQUIRE(resource.node.actions.front().examples.front().requests.front().name.empty());
    REQUIRE(resource.node.actions.front().examples.front().requests.front().description.empty());
    REQUIRE(resource.node.actions.front().examples.front().requests.front().body == "p1\n\n");
}

TEST_CASE("Parse multiple method descriptions", "[resource]")
{
    mdp::ByteBuffer source = \
    "# /1\n"\
    "# GET\n"\
    "p1\n"\
    "# POST\n"\
    "p2\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.size() == 2); // 2x no response

    REQUIRE(resource.node.uriTemplate == "/1");
    REQUIRE(resource.node.description.empty());
    REQUIRE(resource.node.model.name.empty());
    REQUIRE(resource.node.model.body.empty());
    REQUIRE(resource.node.actions.size() == 2);
    REQUIRE(resource.node.actions[0].method == "GET");
    REQUIRE(resource.node.actions[0].description == "p1\n");
    REQUIRE(resource.node.actions[1].method == "POST");
    REQUIRE(resource.node.actions[1].description == "p2\n");
}

TEST_CASE("Parse multiple methods", "[resource]")
{
    mdp::ByteBuffer source = \
    "# /1\n"\
    "A\n"\
    "## GET\n"\
    "B\n"\
    "+ Response 200\n"\
    "    + Body\n\n"\
    "            Code 1\n\n"\
    "## POST\n"\
    "C\n"\
    "+ Request D\n"\
    "+ Response 200\n"\
    "    + Body\n\n"
    "            {}\n\n"\
    "## PUT\n"\
    "E\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.size() == 2); // empty reuqest asset & no response

    REQUIRE(resource.node.uriTemplate == "/1");
    REQUIRE(resource.node.description == "A\n");
    REQUIRE(resource.node.model.name.empty());
    REQUIRE(resource.node.model.body.empty());
    REQUIRE(resource.node.actions.size() == 3);

    REQUIRE(resource.node.actions[0].method == "GET");
    REQUIRE(resource.node.actions[0].description == "B\n");
    REQUIRE(resource.node.actions[0].examples.size() == 1);
    REQUIRE(resource.node.actions[0].examples[0].requests.empty());
    REQUIRE(resource.node.actions[0].examples[0].responses.size() == 1);
    REQUIRE(resource.node.actions[0].examples[0].responses[0].name == "200");
    REQUIRE(resource.node.actions[0].examples[0].responses[0].description.empty());
    REQUIRE(resource.node.actions[0].examples[0].responses[0].body == "Code 1\n");

    REQUIRE(resource.node.actions[1].method == "POST");
    REQUIRE(resource.node.actions[1].description == "C\n");
    REQUIRE(resource.node.actions[1].examples.size() == 1);
    REQUIRE(resource.node.actions[1].examples[0].requests.size() == 1);
    REQUIRE(resource.node.actions[1].examples[0].requests[0].name == "D");
    REQUIRE(resource.node.actions[1].examples[0].requests[0].description.empty());
    REQUIRE(resource.node.actions[1].examples[0].requests[0].description.empty());
    REQUIRE(resource.node.actions[1].examples[0].responses.size() == 1);
    REQUIRE(resource.node.actions[1].examples[0].responses[0].name == "200");
    REQUIRE(resource.node.actions[1].examples[0].responses[0].description.empty());
    REQUIRE(resource.node.actions[1].examples[0].responses[0].body == "{}\n");

    REQUIRE(resource.node.actions[2].method == "PUT");
    REQUIRE(resource.node.actions[2].description == "E\n");
    REQUIRE(resource.node.actions[2].examples.empty());
}

TEST_CASE("Parse description with list", "[resource]")
{
    mdp::ByteBuffer source = \
    "# /1\n"\
    "+ A\n"\
    "+ B\n\n"\
    "p1\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.empty());

    REQUIRE(resource.node.uriTemplate == "/1");
    REQUIRE(resource.node.description == "+ A\n\n+ B\n\np1\n");
    REQUIRE(resource.node.model.name.empty());
    REQUIRE(resource.node.model.body.empty());
    REQUIRE(resource.node.actions.empty());
}

TEST_CASE("Parse resource with a HR", "[resource][block]")
{
    mdp::ByteBuffer source = \
    "# /1\n"\
    "A\n"\
    "---\n"\
    "B\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.empty());

    REQUIRE(resource.node.uriTemplate == "/1");
    REQUIRE(resource.node.description == "A\n---\n\nB\n");
    REQUIRE(resource.node.model.name.empty());
    REQUIRE(resource.node.model.body.empty());
    REQUIRE(resource.node.actions.empty());
}

TEST_CASE("Parse resource method abbreviation", "[resource]")
{
    mdp::ByteBuffer source = \
    "# GET /resource\n"\
    "Description\n"\
    "+ Response 200\n"\
    "    + Body\n\n"\
    "            {}\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.empty());

    REQUIRE(resource.node.name.empty());
    REQUIRE(resource.node.uriTemplate == "/resource");
    REQUIRE(resource.node.actions.size() == 1);
    REQUIRE(resource.node.actions[0].method == "GET");
    REQUIRE(resource.node.actions[0].description == "Description\n");

    REQUIRE(resource.node.actions[0].examples[0].responses.size() == 1);
    REQUIRE(resource.node.actions[0].examples[0].responses[0].description.empty());
    REQUIRE(resource.node.actions[0].examples[0].responses[0].body == "{}\n");
}

TEST_CASE("Parse resource without name", "[resource]")
{
    mdp::ByteBuffer source = "# /resource\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.empty());

    REQUIRE(resource.node.uriTemplate == "/resource");
    REQUIRE(resource.node.name.empty());
    REQUIRE(resource.node.model.name.empty());
    REQUIRE(resource.node.model.body.empty());
    REQUIRE(resource.node.actions.size() == 0);
}

TEST_CASE("Warn about parameters not in URI template", "[resource][source]")
{
    mdp::ByteBuffer source = \
    "# /resource/{id}\n"\
    "+ Parameters\n"\
    "    + olive\n\n"\
    "## GET\n"\
    "+ Parameters\n"\
    "    + cheese\n"\
    "    + id\n\n"\
    "+ Response 204\n\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.size() == 2);
    REQUIRE(resource.report.warnings[0].code == LogicalErrorWarning);
    REQUIRE(resource.report.warnings[1].code == LogicalErrorWarning);

    REQUIRE(resource.node.parameters.size() == 1);
    REQUIRE(resource.node.parameters[0].name == "olive");
    REQUIRE(resource.node.actions.size() == 1);
    REQUIRE(resource.node.actions[0].examples.size() == 1);
    REQUIRE(resource.node.actions[0].examples[0].responses.size() == 1);
}

TEST_CASE("Parse nameless resource with named model", "[resource][model][source]")
{
    mdp::ByteBuffer source = \
    "# /message\n"\
    "+ Super Model\n"\
    "\n"\
    "        AAA\n"\
    "\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.empty());

    REQUIRE(resource.node.model.name == "Super");
    REQUIRE(resource.node.model.body == "AAA\n");
    REQUIRE(resource.node.actions.empty());
}

TEST_CASE("Parse nameless resource with nameless model", "[resource][model][source]")
{
    mdp::ByteBuffer source = \
    "# /message\n"\
    "+ Model\n"\
    "\n"\
    "        AAA\n"\
    "\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == SymbolError);
    REQUIRE(resource.report.warnings.empty());

    REQUIRE(resource.node.model.name.empty());
}

TEST_CASE("Parse named resource with nameless model", "[resource][model][source]")
{
    mdp::ByteBuffer source = \
    "# Message [/message]\n"\
    "+ Model\n\n"\
    "        AAA\n"\
    "\n"\
    "## Retrieve a message [GET]\n"\
    "+ Response 200\n\n"\
    "    [Message][]\n\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.empty());

    REQUIRE(resource.node.model.name == "Message");
    REQUIRE(resource.node.model.body == "AAA\n");
    REQUIRE(resource.node.actions.size() == 1);
    REQUIRE(resource.node.actions[0].name == "Retrieve a message");
    REQUIRE(resource.node.actions[0].method == "GET");
    REQUIRE(resource.node.actions[0].examples.size() == 1);
    REQUIRE(resource.node.actions[0].examples[0].responses.size() == 1);
    REQUIRE(resource.node.actions[0].examples[0].responses[0].name == "200");
    REQUIRE(resource.node.actions[0].examples[0].responses[0].body == "AAA\n");
}

TEST_CASE("Parse model with unrecognised resource", "[resource][model]")
{
    mdp::ByteBuffer source = \
    "# Resource [/1]\n\n"\
    "+ Model (plain/text)\n\n"\
    "        AAA\n\n"\
    "## Retrieve a resource [GET]\n\n"\
    "+ Response 200\n\n"\
    "    + Headers\n\n"\
    "            X-Header: A\n\n"\
    "    + Body\n\n"\
    "            [Resource][]";

    Resource resource;
    Report report;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, report, resource);

    REQUIRE(report.error.code == Error::OK);
    REQUIRE(report.warnings.size() == 1);
    REQUIRE(report.warnings[0].code == IgnoringWarning);

    REQUIRE(resource.model.name == "Resource");
    REQUIRE(resource.model.body == "AAA\n");
    REQUIRE(resource.actions.size() == 1);
    REQUIRE(resource.actions[0].name == "Retrieve a resource");
    REQUIRE(resource.actions[0].method == "GET");
    REQUIRE(resource.actions[0].examples.size() == 1);
    REQUIRE(resource.actions[0].examples[0].responses.size() == 1);
    REQUIRE(resource.actions[0].examples[0].responses[0].name == "200");
    REQUIRE(resource.actions[0].examples[0].responses[0].body == "[Resource][]\n");
    REQUIRE(resource.actions[0].examples[0].responses[0].description == "");
}

TEST_CASE("Parse named resource with nameless model but reference a non-existing model", "[resource]")
{
    mdp::ByteBuffer source = \
    "# Posts [/posts]\n"\
    "+ Model\n\n"\
    "        {}\n"\
    "\n"\
    "## List [GET]\n"\
    "+ Response 200\n\n"\
    "    [Post][]\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == SymbolError);
    REQUIRE(resource.report.warnings.empty());
}

TEST_CASE("Parse root resource", "[resource]")
{
    mdp::ByteBuffer source = "# API Root [/]\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.empty());

    REQUIRE(resource.node.name == "API Root");
    REQUIRE(resource.node.uriTemplate == "/");
    REQUIRE(resource.node.actions.empty());
}

TEST_CASE("Parse resource with invalid URI Tempalte", "[resource]")
{
    mdp::ByteBuffer source = "# Resource [/id{? limit}]\n";
    
    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);
    
    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.size() == 1);
    REQUIRE(resource.report.warnings[0].code == URIWarning);
    
    REQUIRE(resource.node.name == "Resource");
    REQUIRE(resource.node.uriTemplate == "/id{? limit}");
    REQUIRE(resource.node.actions.empty());
}

TEST_CASE("Deprecated resource and action headers", "[resource]")
{
    mdp::ByteBuffer source = \
    "# /\n"\
    "+ Headers\n\n"\
    "        header1: value1\n\n"\
    "## GET\n"\
    "+ Headers\n\n"\
    "        header2: value2\n\n"\
    "+ Response 200\n"\
    "    + Headers\n\n"\
    "            header3: value3\n\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.size() == 2);
    REQUIRE(resource.report.warnings[0].code == DeprecatedWarning);
    REQUIRE(resource.report.warnings[1].code == DeprecatedWarning);

    REQUIRE(resource.node.headers.empty());
    REQUIRE(resource.node.actions.size() == 1);
    REQUIRE(resource.node.actions[0].headers.empty());
    REQUIRE(resource.node.actions[0].examples.size() == 1);
    REQUIRE(resource.node.actions[0].examples[0].responses.size() == 1);
    REQUIRE(resource.node.actions[0].examples[0].responses[0].headers.size() == 3);
    REQUIRE(resource.node.actions[0].examples[0].responses[0].headers[0].first == "header1");
    REQUIRE(resource.node.actions[0].examples[0].responses[0].headers[0].second == "value1");
    REQUIRE(resource.node.actions[0].examples[0].responses[0].headers[1].first == "header2");
    REQUIRE(resource.node.actions[0].examples[0].responses[0].headers[1].second == "value2");
    REQUIRE(resource.node.actions[0].examples[0].responses[0].headers[2].first == "header3");
    REQUIRE(resource.node.actions[0].examples[0].responses[0].headers[2].second == "value3");
}

TEST_CASE("bug fix for recognition of model as a part of other word or as a quote, issue #92 and #152", "[model]")
{
    mdp::ByteBuffer source = \
    "## Resource [/resource]\n"\
    "### Attributes\n"\
    "- A\n"\
    "- Cmodel\n"\
    "- Single data model for all exchange data\n"\
    "- `model`\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.size() == 0);

    REQUIRE(resource.node.name == "Resource");
    REQUIRE(resource.node.description == "### Attributes\n\n- A\n\n- Cmodel\n\n- Single data model for all exchange data\n\n- `model`\n");
}

TEST_CASE("Parse resource with multi-word named model", "[resource][model]")
{
    mdp::ByteBuffer source = \
    "# My Resource [/resource]\n\n"\
    "Awesome description\n\n"\
    "+ a really good name Model (text/plain)\n\n"\
    "        body of the `model`\n";

    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);

    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.empty());

    REQUIRE(resource.node.model.name == "a really good name");
    REQUIRE(resource.node.model.body == "body of the `model`\n");
    REQUIRE(resource.node.actions.empty());
}

TEST_CASE("Dangling transaction example assets", "[resource]")
{
    mdp::ByteBuffer source = \
    "# A [/a]\n"\
    "## GET\n"\
    "+ Request A\n"\
    "\n"\
    "```js\n"\
    "dangling request body\n"\
    "```\n"\
    "\n"\
    "+ Response 200\n"\
    "\n"\
    "```\n"\
    "dangling response body\n"\
    "```\n";
    
    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);
    
    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.size() == 3);
    REQUIRE(resource.report.warnings[0].code == EmptyDefinitionWarning);
    REQUIRE(resource.report.warnings[1].code == IndentationWarning);
    REQUIRE(resource.report.warnings[2].code == IndentationWarning);
    
    REQUIRE(resource.node.name == "A");
    REQUIRE(resource.node.uriTemplate == "/a");
    REQUIRE(resource.node.actions.size() == 1);
    
    REQUIRE(resource.node.actions[0].method == "GET");
    REQUIRE(resource.node.actions[0].examples.size() == 1);
    REQUIRE(resource.node.actions[0].examples[0].requests.size() == 1);
    REQUIRE(resource.node.actions[0].examples[0].requests[0].name == "A");
    REQUIRE(resource.node.actions[0].examples[0].requests[0].body == "dangling request body\n\n");
    
    REQUIRE(resource.node.actions[0].examples[0].responses.size() == 1);
    REQUIRE(resource.node.actions[0].examples[0].responses[0].name == "200");
    REQUIRE(resource.node.actions[0].examples[0].responses[0].body == "dangling response body\n\n");
}

TEST_CASE("Body list item in description", "[resource][regression][#190]")
{
    mdp::ByteBuffer source = \
    "## GET /A\n"\
    "Lorem Ipsum\n"\
    "\n"\
    "+ Body\n"\
    "\n"\
    "    { ... }\n"\
    "\n"\
    "+ Response 200\n";
    
    ParseResult<Resource> resource;
    SectionParserHelper<Resource, ResourceParser>::parse(source, ResourceSectionType, resource);
    
    REQUIRE(resource.report.error.code == Error::OK);
    REQUIRE(resource.report.warnings.empty());
    
    REQUIRE(resource.node.actions.size() == 1);
    REQUIRE(resource.node.actions[0].description == "Lorem Ipsum\n\n+ Body\n\n    { ... }\n\n");
}
