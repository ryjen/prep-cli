#ifndef INCLUDE_ARGUMENT_RESOLVER
#define INCLUDE_ARGUMENT_RESOLVER


namespace arg3
{
    namespace cpppm
    {
        class argument_resolver
        {
        public:
            static const int UNKNOWN = 255;
            argument_resolver(const std::string &arg);
            argument_resolver();

            void set_arg(const std::string &arg);
            std::string arg() const;

            int resolve_package() const;

        private:
            std::string arg_;
        };
    }
}

#endif

