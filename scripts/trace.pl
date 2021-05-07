#!/usr/bin/env perl
# SPDX-License-Identifier: GPL-2.0

# This file is part of libvfn.
#
# Copyright (C) 2022 The libvfn Authors. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

use strict;
use warnings;

sub usage {
	print "usage: $0 [--mode {header,source}] EVENTSFILE...\n";
	exit 1;
}

my @events;
my $mode = "header";

while (@ARGV && $ARGV[0] =~ m/^(--?.*)/) {
	my $arg = $1;
	shift @ARGV;

	if ($arg eq "-m" || $arg eq "--mode") {
		$mode = shift @ARGV;
	} else {
		usage();
	}
}

sub collect($) {
	my ($file) = @_;

	open(IN, "<$file") or die "cannot open '$file'";

	while (<IN>) {
		continue if (/^#/);

		unless (/^(-)?(\w+)$/) {
			die "parse error";
		}

		push @events, {
			disabled => $1 ? 1 : 0,
			name => $2,
		};
	}
}

sub output_header() {
	foreach my $event (@events) {
		printf "#define TRACE_%s \"%s\"\n", $event->{"name"}, lc $event->{"name"};
	}

	print "\n";

	foreach my $event (@events) {
		printf "#define TRACE_%s_DISABLED %d\n", $event->{"name"}, $event->{"disabled"};
	}

	print "\n";

	foreach my $event (@events) {
		printf "extern bool TRACE_%s_ACTIVE;\n", $event->{"name"};
	}
}

sub output_source() {
	print <<EOF;
#include <stdbool.h>

#include "vfn/trace/events.h"
#include "trace.h"

EOF
	foreach my $event (@events) {
		printf "bool TRACE_%s_ACTIVE;\n", $event->{"name"};
	}

	print "\n";

	print "struct trace_event trace_events[] = {\n";

	foreach my $event (@events) {
		printf "\t{\"%s\", &TRACE_%s_ACTIVE},\n", lc $event->{"name"}, $event->{"name"};
	}

	print "};\n\n";

	printf "unsigned int TRACE_NUM_EVENTS = %d;\n", scalar @events;
}

if ($#ARGV == -1) {
	usage();
}

foreach (@ARGV) {
	chomp;
	collect($_);
}

if ($mode eq "header") {
	output_header();
} elsif ($mode eq "source") {
	output_source();
} else {
	usage();
}
