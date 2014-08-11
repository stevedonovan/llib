<html>
<head>
</head>
<body>
<h2>$(title)</h2>
$(with name|<p>Welcome $(first) $(last)!</p>|)
<ul>
$(for items|<li>
 $(if url|
  <a src='$(url)'>$(title)</a>
 |)$(else|
  <b>$(title)</b>
 |)
 </li>|)
</ul>
</body>
</html>
$(test title ||)

