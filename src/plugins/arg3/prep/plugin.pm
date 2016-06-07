# Ryan Jennings 2016
package arg3::prep::plugin;
use strict;
use warnings;
use Exporter qw(import);

our @EXPORT_OK = qw(get_build_params get_param get_package_params);

sub parse_env_vars
{
    while (<STDIN>) {
        chomp;

        if (lc $_ eq "end") {
            last;
        }

        my ($k, $v) = split /=/, $_, 2;
        if (defined $k && defined $v) {
            $v =~ s/^(['"])(.*)\1/$2/; #' fix highlighter
            $v =~ s/\$([a-zA-Z]\w*)/$ENV{$1}/g;
            $v =~ s/`(.*?)`/`$1`/ge; #dangerous
            $ENV{$k} = $v;
        }
    }
}

sub get_param
{
    my $param = <STDIN>;

    chomp($param);

    return $param;
}

sub get_build_params
{
    my $package = get_param;
    my $version = get_param;
    my $sourcePath = get_param;
    my $buildPath = get_param;
    my $installPath = get_param;
    my $buildOpts = get_param;

    parse_env_vars;

    return ($package, $version, $sourcePath, $buildPath, $installPath, $buildOpts);
}

sub get_package_params
{
    my $package = get_param;
    my $version = get_param;

    return ($package, $version);
}

1;
