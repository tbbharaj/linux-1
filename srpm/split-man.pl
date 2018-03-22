#!/usr/bin/perl
# Used to generate man pages
# Source: $(SRC)/Documentation/kernel-doc-nano-HOWTO.txt

if ($#ARGV < 0) {
	die "where do I put the results?\n";
}

mkdir $ARGV[0],0777;
$state = 0;
while (<STDIN>) {
	if (/^\.TH \"[^\"]*\" 9 \"([^\"]*)\"/) {
		if ($state == 1) { close OUT }
		$state = 1;
		$fn = "$ARGV[0]/$1.9";
		print STDERR "Creating $fn\n";
		open OUT, ">$fn" or die "can't open $fn: $!\n";
		print OUT $_;
	} elsif ($state != 0) {
		print OUT $_;
	}
}

close OUT;
