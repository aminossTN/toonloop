import feedparser

xml_tpl = """
<?xml version="1.0" encoding="utf-8"?>
<rss version="2.0">
<channel>
  <title>Sample Feed</title>
  <description>For documentation &lt;em&gt;only&lt;/em&gt;</description>
  <link>http://example.org/</link>
  <language>en</language>
  <copyright>Copyright 2004, Mark Pilgrim</copyright>
  <managingEditor>editor@example.org</managingEditor>
  <webMaster>webmaster@example.org</webMaster>
  <pubDate>Sat, 07 Sep 2002 00:00:01 GMT</pubDate>
  <category domain="Syndic8">1024</category>
  <category domain="dmoz">Top/Society/People/Personal_Homepages/P/</category>
  <generator>Sample Toolkit</generator>
  <docs>http://feedvalidator.org/docs/rss2.html</docs>
  <cloud domain="rpc.example.com" port="80" path="/RPC2" registerProcedure="pingMe" protocol="soap"/>
  <ttl>60</ttl>
  <image>
    <url>http://example.org/banner.png</url>
    <title>Example banner</title>
    <link>http://example.org/</link>
    <width>80</width>
    <height>15</height>
  </image>
  <textInput>
    <title>Search</title>
    <description>Search this site:</description>
    <name>q</name>
    <link>http://example.org/mt/mt-search.cgi</link>
  </textInput>
  <item>
    <title>First item title</title>
    <link>http://example.org/item/1</link>
    <description>Watch out for &lt;span style="background: url(javascript:window.location='http://example.org/')"&gt;nasty tricks&lt;/span&gt;</description>
    <author>mark@example.org</author>
    <category>Miscellaneous</category>
    <comments>http://example.org/comments/1</comments>
    <enclosure url="http://example.org/audio/demo.mp3" length="1069871" type="audio/mpeg"/>
    <guid>http://example.org/guid/1</guid>
    <pubDate>Thu, 05 Sep 2002 00:00:01 GMT</pubDate>
  </item>
</channel>
</rss>
"""

if __name__ == '__name__':
    import pprint
    global xml_tpl
    d = feedparser.parse(xml_tpl)

    print "RESULT:"    
    pprint.pprint(d)

