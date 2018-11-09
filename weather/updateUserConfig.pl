use utf8;
use open ':encoding(utf8)';
utf8::upgrade($out);

my $out = `lynx -dump -accept_all_cookies https://your_url_here`;

my $temp = $1 if $out =~ /Ощущается\s+как\s+\+?([0-9-]+)/s; 

die "no temp value in $out" unless $temp;



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

