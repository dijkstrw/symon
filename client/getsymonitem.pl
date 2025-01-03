#!/usr/bin/perl
#
# Example program using SymuxClient
#

use SymuxClient;

die "Usage: $0 <measured host> <stream> <item>
Example: $0 127.0.0.1 'cpu(0)' user\n" if ($#ARGV < 2);

my $sc = new SymuxClient();

print $sc->getitem( $ARGV[0], $ARGV[1], $ARGV[2] );
