#!/usr/bin/perl
#
# $Id: getsymonitem.pl,v 1.1 2002/11/08 15:40:25 dijkstra Exp $
# 
# Example program of how to use SymuxClient

use SymuxClient;

die "Usage: $0 <symux host> <symux port> <measured host> <stream> <item>
Example: $0 127.0.0.1 2100 127.0.0.1 'cpu(0)' user\n" if ($#ARGV < 4);

my $sc = new SymuxClient( host => $ARGV[0],
			  port => $ARGV[1]);

print $sc->getitem( $ARGV[2], $ARGV[3], $ARGV[4] );
			  