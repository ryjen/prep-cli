
#include "package_config.h"
#include <fstream>
#include <sstream>

namespace arg3
{
    namespace cpppm
    {
        package_config::package_config() : values_(NULL)
        {

        }

        package_config::~package_config()
        {
            if (values_ != NULL)
            {
                json_object_put(values_);
                values_ = NULL;
            }
        }
        bool package_config::load(const string &filename)
        {
            ifstream file;

            file.open(filename);

            if (!file.is_open())
            {
                printf("unabel to open %s\n", filename.c_str());
                return false;
            }

            std::ostringstream os;

            file >> os.rdbuf();

            values_ = json_tokener_parse(os.str().c_str());

            if (values_ == NULL)
            {
                printf("invalid configuration for %s\n", os.str().c_str());
            }
            return values_ != NULL;
        }

        string package_config::name() const
        {
            json_object *obj = json_object_object_get(values_, "name");
            return json_object_get_string(obj);
        }
        string package_config::include_dir() const
        {
            json_object *obj = json_object_object_get(values_, "include_dir");
            return json_object_get_string(obj);
        }
        string package_config::source_dir() const
        {
            json_object *obj = json_object_object_get(values_, "source_dir");
            return json_object_get_string(obj);
        }
        vector<string> package_config::get_array(const std::string &name) const
        {
            json_object *obj = json_object_object_get(values_, name.c_str());

            int size = json_object_array_length(obj);

            vector<string> values(size);

            for (int i = 0; i < size; i++)
            {
                json_object *option = json_object_array_get_idx(obj, i);
                values.push_back( json_object_get_string(option) );
            }
            return values;
        }

        vector<string> package_config::link_options() const
        {
            return get_array("link_options");
        }
        vector<string> package_config::compile_options() const
        {
            return get_array("compile_options");
        }
    }
}