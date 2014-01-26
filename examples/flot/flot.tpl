<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
 <head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
    <title>llib Flot</title>
    <!--[if lte IE 8]><script language="javascript" type="text/javascript" src="@(flot)/excanvas.min.js"></script><![endif]-->
    <script language="javascript" type="text/javascript" src="@(jquery)"></script>
    <script language="javascript" type="text/javascript" src="@(flot)/jquery.flot.js"></script>
 </head>
<body>
@(for plots |
<h2>@(title)</h2>
<div id="@(div)" style="width:@(width)px;height:@(height)px"></div>
|)
<script type="text/javascript">
$(function () {
@(for plots |
  $.plot( $("#@(div)"),
    @(data),
    @(options)
 )
|)
});
</script>
 </body>
</html>
