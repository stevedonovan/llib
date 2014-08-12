$(def "H2" <h2>$(_)</h2>)
<html>
<head>
</head>
<body>
<h2>$(title)</h2>
$(with name <p>Welcome $(first) $(last)!</p>)
<ul>
$(for items <li>
 $(if url 
  <a src='$(url)'>$(title)</a>
 )$(else 
  <b>$(title)</b>
 )
 </li>)
</ul>
$(test "hello world") $(with extra $(1) is $(2))
$(H2 "and more")
</body>
</html>


