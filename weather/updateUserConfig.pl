use utf8;
use open ':encoding(utf8)';
utf8::upgrade($out);

my $out = `lynx -dump -accept_all_cookies https://your_url_here`;

my $temp = $1 if $out =~ /Ощущается\s+как\s+(\S+)/s;
$temp =~ s/\s+//g;
$temp =~ s/(\d+).*$/$1/s;
$temp = "-$1" unless $temp =~ /^\d/;

die "no temp value ($temp) in $out" unless length($temp);


open(F, ">config/userConfig.json");
print F <<DOC;
{
        "temp": {
                "inside": 23,
                "outside": $temp
        }
}
DOC

close F;

