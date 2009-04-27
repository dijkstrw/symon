# $Id: SymuxClient.pm,v 1.15 2008/07/22 17:35:49 dijkstra Exp $
#
# Copyright (c) 2001-2008 Willem Dijkstra
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#    - Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    - Redistributions in binary form must reproduce the above
#      copyright notice, this list of conditions and the following
#      disclaimer in the documentation and/or other materials provided
#      with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

package SymuxClient;

use Carp;
use IO::Socket;

my $streamitem =
    {cpu    => {user => 1, nice => 2, system => 3, interrupt => 4, idle => 5},
     cpuiow => {user => 1, nice => 2, system => 3, interrupt => 4, idle => 5, iowait => 6},
     mem    => {real_active => 1, real_total => 2, free => 3, swap_used => 4,
	        swap_total =>5},
     if     => {packets_in => 1, packets_out => 2, bytes_in => 3, bytes_out => 4,
	        multicasts_in => 5, multicasts_out => 6, errors_in => 7,
	        errors_out => 8, collisions => 9, drops => 10},
     io1    => {total_transfers => 1, total_seeks => 2, total_bytes => 3},
     pf     => {bytes_v4_in => 1, bytes_v4_out => 2, bytes_v6_in => 3,
	        bytes_v6_out => 4, packets_v4_in_pass => 5,
	        packets_v4_in_drop => 6, packets_v4_out_pass => 7,
	        packets_v4_out_drop => 8, packets_v6_in_pass => 9,
	        packets_v6_in_drop => 10, packets_v6_out_pass => 11,
	        packets_v6_out_drop => 12, states_entries => 13,
	        states_searches => 14, states_inserts => 15,
	        states_removals => 16, counters_match => 17,
	        counters_badoffset => 18, counters_fragment => 19,
	        counters_short => 20, counters_normalize => 21,
	        counters_memory => 22},
     debug  => {debug0 => 1, debug1 => 2, debug3 => 3, debug4 => 4, debug5 => 5,
	        debug6 => 6, debug7 => 7, debug8 => 8, debug9 => 9,
	        debug10 => 10, debug11 => 11, debug12 => 12, debug13 => 13,
	        debug14 => 14, debug15 => 15, debug16 => 16, debug17 => 17,
	        debug18 => 18, debug19 => 19},
     proc   => {number => 1, uticks => 2, sticks => 3, iticks => 4, cpusec => 5,
	        cpupct => 6, procsz => 7, rsssz => 8},
     mbuf   => {totmbufs => 1, mt_data => 2, mt_oobdata => 3, mt_control => 4,
	        mt_header => 5, mt_ftable => 6, mt_soname => 7, mt_soopts => 8,
	        pgused => 9, pgtotal => 10, totmem => 11, totpct => 12,
	        m_drops => 13, m_wait => 14, m_drain => 15 },
     sensor => {value => 1},
     io     => {total_rxfers => 1, total_wxfers => 2, total_seeks => 3,
		total_rbytes => 4, total_wbytes => 5 },
     pfq    => {sent_bytes => 1, sent_packets => 2, drop_bytes => 3,
		drop_packets => 4},
     df     => {blocks => 1, bfree => 2, bavail => 3, files => 4, ffree => 5,
		syncwrites => 6, asyncwrites => 7},
     smart  => {read_error_rate => 1, reallocated_sectors => 2, spin_retries => 3,
                air_flow_temp => 4, temperature => 5, reallocations => 6,
                current_pending => 7, uncorrectables => 8,
                soft_read_error_rate => 9, g_sense_error_rate => 10,
                temperature2 => 10, free_fall_protection => 11}
};

sub new {
    my ($class, %arg) = @_;
    my $self;

    (defined $arg{host} && defined $arg{port}) or
	croak "error: need a host and a port to connect to.";

    ($self->{host}, $self->{port}) = ($arg{host}, $arg{port});

    $self->{retry} = (defined $arg{retry}? $arg{retry} : 10);

    bless($self, $class);

    $self->connect();

    return $self;
}

sub DESTROY {
    my $self = shift;

    if (defined $self->{sock}) {
	close($self->{sock});
    }
}

sub connect {
    my $self = shift;

    if (defined $self->{sock} && $self->{sock}->connected() ne '') {
	return;
    } else {
	close($self->{sock});
    }

    $self->{sock} = new
      IO::Socket::INET(PeerAddr => $self->{host},
		       PeerPort => $self->{port},
		       Proto => "tcp",
		       Type => SOCK_STREAM)
	  or croak "error: could not connect to $self->{host}:$self->{port}";
}

sub getdata {
    my $self = shift;
    my $sock;
    my $data;
    my $tries;

    $tries = 0;

    while (1) {
	$self->connect();
	$sock = $self->{sock};
	$data = <$sock>;
	if ((index($data, "\012") != -1) && (index($data, ';') != -1)) {
	    $self->{rawdata} = $data;
	    return $data;
	} else {
	    croak "error: tried to get data $tries times and failed"
		if (++$tries == $self->{retry});
	}
    }
}

sub parse {
    my $self = shift;
    my ($stream, @streams, $name, $arg, @data, $number);

    $self->getdata() if ! defined $self->{rawdata};

    $number = 0;
    undef $self->{data};
    undef $self->{datasource};
    chop $self->{rawdata}; # remove ';\n'
    chop $self->{rawdata};

    @streams = split(/;/, $self->{rawdata});
    croak "error: expected a symux dataline with ';' delimited streams"
	if ($#streams < 1);

    $self->{datasource} = shift @streams;

    foreach $stream (@streams) {
	($name, $arg, @data) = split(':', $stream);

	croak "error: expected a symux stream with ':' delimited values"
	    if ($#data < 1);

	$name .= '('.$arg.')' if ($arg ne '');

	@{$self->{data}{$name}} = @data;
	$number++;
    }

    $self->{rawdata} = '';
    return $number;
}

sub getcacheditem {
    my ($self, $host, $streamname, $item) = @_;
    my ($streamtype, @addr, $element);

    return undef if not defined $self->{datasource};
    return undef if (($host ne $self->{datasource})  &&
		     ($host ne "*"));

    croak "error: source $host does not contain stream $streamname"
	if not defined $self->{data}{$streamname};

    ($streamtype = $streamname) =~ s/^([a-z]+).*/\1/;

    if ($item eq 'timestamp') {
	$element = 0;
    } elsif (not defined $$streamitem{$streamtype}{$item}) {
	croak "error: unknown stream item '$item' - check symux(8) for names";
    } else {
	$element = $$streamitem{$streamtype}{$item};
    }

    return $self->{data}{$streamname}[$element];
}

sub getitem {
    my ($self, $host, $streamname, $item) = @_;
    my $data;
    my %hosts = ();

    undef $data;
    while (! defined $data) {
	$self->getdata();
	$self->parse();
	$hosts{$self->{datasource}} += 1;
	if ($hosts{$self->{datasource}} > $self->{retry}) {
	    croak "error: seen a lot of data ($tries strings), but none that matches $host";
	}
	$data = $self->getcacheditem($host, $streamname, $item);
	return $data if defined $data;
	$tries++;
    }
}

sub source {
    my $self = shift;

    return $self->{datasource};
}
1;

__END__

=head1 NAME

SymuxClient - Object client interface for symux

=head1 SYNOPSIS

use SymuxClient;

=head1 DESCRIPTION

C<SymuxClient> provides an object client interface for listening to, and
parsing of, symux data.

=head1 CONSTRUCTOR

=over 4

=item new ( ARGS )

Creates a new C<SymuxClient> object that holds both the connection to a symux
and data it receives from that connection. Arguments are:

    host           dotted quad ipv4 hostaddress
    port           tcp port that symux is on
    retry*         maximum number of retries; used to limit number
		   of connection attempts and number of successive
		   read attempts

Arguments marked with * are optional.

Example:
    $sc = new SymuxClient(host => '127.0.0.1',
			  port => 2100);

=back

=head2 METHODS

=item getitem (host, stream, item)

Refresh the measured data and get an item from a stream for a particular
host. Note that successive calls for this function deal with successive
measurements of B<symon>. Set C<host> to '*' if data about any host is of
interest. Any errors are sent out on STDOUT prepended with 'error: '.

=item getcacheditem (host, stream, item)

Get an item from a stream for a particular host. Returns C<undef> if no data is
cached, or if the data cached does not match the B<host>. Can be called
multiple times to obtain items from the same measurement. Set C<host> to '*' if
data about any host is of interest. Any errors are sent out on STDOUT prepended
with 'error: '.

=item getsource ()

Get the symon source host of the currently cached information. Usefull to see
what host's data getcacheditem is working on.

Example:
    $sc = new SymuxClient(host => 127.0.0.1,
			  port => 2100);

    print $sc->getitem("127.0.0.1", "if(de0)",
		       "packets_out");

    # get more data from that measurement
    print $sc->getcacheditem("127.0.0.1","if(de0)",
			     "packets_in");

    # start a new measurement
    print $sc->getitem("*", "if(de0)",
		       "packets_out");
    # which hosts packets_out was that?
    print $sc->getsource();

=cut
