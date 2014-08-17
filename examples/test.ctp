$(def "H2": <h2>$(_)</h2>)
<html>
<head>
</head>
<body>
<h2>$(title)</h2>
$(with name: <p>Welcome $(first) $(last)!</p>)
<ul> $(for items:
  <li> $(if url:
  <a src='$(url)'>$(title)</a>
 )$(else: 
  <b>$(title)</b>
 )  </li>)
</ul>
$(test "hello world") $(extra.1) is $(extra.2) $(name.first) $(items.1.title)
$(with items.1:  Title is $(title), url is $(url))
$(H2 "and more")
</body>
</html>


