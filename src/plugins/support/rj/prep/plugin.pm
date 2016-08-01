# Ryan Jennings 2016
package rj::prep::plugin;
use strict;
use warnings;
use Exporter qw(import);

our @EXPORT_OK = qw(new execute read_build_params read_package_params read_resolve_params add_return_value);

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

sub read_param
{
    my $param = <STDIN>;

    chomp($param);

    return $param;
}

sub read_build_params
{
    my $package = read_param;
    my $version = read_param;
    my $sourcePath = read_param;
    my $buildPath = read_param;
    my $installPath = read_param;
    my $buildOpts = read_param;

    parse_env_vars;

    return ($package, $version, $sourcePath, $buildPath, $installPath, $buildOpts);
}

sub read_package_params
{
    my $package = read_param;
    my $version = read_param;
    my $repository = read_param;

    return ($package, $version, $repository);
}

sub read_resolve_params
{
    my $path = read_param;
    my $location = read_param;

    return ($path, $location);
}

sub add_return_value
{
    my $value = shift;

    print "RETURN $value\n";
}

sub on_load
{
    my $self = shift;

    if (@_) {
        $self->{on_load} = shift;  
    }

    return $self->{on_load};
}

sub on_build
{
    my $self = shift;
    if (@_) {
        $self->{on_build} = shift;
    }

    return $self->{on_build};
}

sub on_install
{
    my $self = shift;

    if (@_) {
        $self->{on_install} = shift;
    }

    return $self->{on_install};
}

sub on_remove
{
    my $self = shift;

    if (@_) {
        $self->{on_remove} = shift;
    }

    return $self->{on_remove};
}

sub on_resolve
{
    my $self = shift;

    if (@_) {
        $self->{on_resolve} = shift;
    }

    return $self->{on_resolve};
}

sub new
{
    my $class = shift;
    
    my $self = { };
    bless $self, $class;
    $self->{on_load} = sub { return 1; };
    $self->{on_install} = sub { return 1; };
    $self->{on_remove} = sub { return 1; };
    $self->{on_build} = sub { return 1; };
    $self->{on_resolve} = sub { return 1; };
    return $self;
}

sub execute
{   
    my $self = shift;
    my $command = read_param;

    if ($command eq "load") {
        return $self->{on_load}->();
    }

    if ($command eq "install") {
        return $self->{on_install}->();
    }

    if ($command eq "remove") {
        return $self->{on_remove}->();
    }

    if ($command eq "build") {
        return $self->{on_build}->();
    }

    if ($command eq "resolve") {
        return $self->{on_resolve}->();
    }

    return 1;
}

1;
