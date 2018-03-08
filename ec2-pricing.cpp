#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/writer.h"
#include <cstdio>
#include <map>
using namespace rapidjson;

const Value& getFirstWithKey(const Value& val, const char* key) {
    Value::ConstMemberIterator itr = val.MemberBegin();
    if (itr == val.MemberEnd()) return Value().Move();
    const Value& firstVal = itr->value;
    itr = firstVal.FindMember(key);
    if (itr == firstVal.MemberEnd()) return Value().Move();
    return itr->value;
}

bool checkstringValue(const Value& obj, const char* key, const char* value) {
    Value::ConstMemberIterator itr = obj.FindMember(key);
    return (itr != obj.MemberEnd() && itr->value == value);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, R"help(Missing path argument.
Filepath to AWS pricing JSON file is required.
AWS pricing JSON file can be downloaded here:
  https://pricing.us-east-1.amazonaws.com/offers/v1.0/aws/index.json
See: https://aws.amazon.com/blogs/aws/new-aws-price-list-api/
)help");
        return 2;
    }
    const std::map<const char* , const char*> LOCATIONS = {
    { "us-east-1.json"      , "US East (N. Virginia)"     },
    { "us-east-2.json"      , "US East (Ohio)"            },
    { "us-west-1.json"      , "US West (N. California)"   },
    { "us-west-2.json"      , "US West (Oregon)"          },
    { "ca-central-1.json"   , "Canada (Central)"          },
    { "eu-west-1.json"      , "EU (Ireland)"              },
    { "eu-central-1.json"   , "EU (Frankfurt)"            },
    { "eu-west-2.json"      , "EU (London)"               },
    { "ap-northeast-1.json" , "Asia Pacific (Tokyo)"      },
    { "ap-northeast-2.json" , "Asia Pacific (Seoul)"      },
    { "ap-southeast-1.json" , "Asia Pacific (Singapore)"  },
    { "ap-southeast-2.json" , "Asia Pacific (Sydney)"     },
    { "ap-south-1.json"     , "Asia Pacific (Mumbai)"     },
    { "sa-east-1.json"      , "South America (Sao Paulo)" },
    };

    FILE* fp = fopen(argv[1], "r");
    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    Document d;
    d.ParseStream(is);
    fclose(fp);

    Value* onDemandPricing = Pointer("/terms/OnDemand").Get(d);

    for (auto const& location : LOCATIONS) {
        const char* filename = location.first;
        const char* locationFull = location.second;
        printf("Generating %s [%s]...\n", filename, locationFull);

        Document out;
        out.SetObject();

        for (auto& m : d["products"].GetObject()) {
            if (!m.value.IsObject()) continue; // some of objects has been moved to out
            if (!checkstringValue(m.value, "productFamily", "Compute Instance")) continue;
            if (!m.value.HasMember("attributes")) continue;

            auto& attributes = m.value["attributes"];
            if ( !checkstringValue(attributes, "tenancy"        , "Shared") ) continue;
            if ( !checkstringValue(attributes, "preInstalledSw" , "NA") ) continue;
            if ( !checkstringValue(attributes, "operatingSystem", "Linux") ) continue;
            if ( !checkstringValue(attributes, "location"       , locationFull) ) continue;

            if (onDemandPricing) {
                Value::ConstMemberIterator itr = onDemandPricing->FindMember(m.value["sku"].GetString());
                if (itr != onDemandPricing->MemberEnd()) {
                    const auto& val = getFirstWithKey(itr->value, "priceDimensions");
                    if (!val.IsNull()) {
                        const auto& priceVal = getFirstWithKey(val, "pricePerUnit");
                        itr = priceVal.FindMember("USD");
                        if (itr != priceVal.MemberEnd()) {
                            Document::AllocatorType& a = d.GetAllocator();
                            Value pricing(itr->value, a);
                            m.value.AddMember("pricing", pricing, a);
                        }
                    }
                }
            }

            Document::AllocatorType& a = out.GetAllocator();
            Value instanceType(attributes["instanceType"], a);
            out.AddMember(instanceType, m.value, a);
        }

        fp = fopen(filename, "w");
        char writeBuffer[65536];
        FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
        Writer<FileWriteStream> writer(os);
        out.Accept(writer);
        fclose(fp);
    }

    printf("done.\n");

    return 0;
}
