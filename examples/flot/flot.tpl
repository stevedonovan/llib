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
    function text_marking(plot,id,xp,yp, text) {
 		var o = plot.pointOffset({ x: xp, y: yp});
		// Append it to the placeholder that Flot already uses for positioning
		$("#"+id).append("<div style='position:absolute;left:" + (o.left + 4) + "px;top:" + o.top + "px;color:#666;font-size:smaller'>" + text + "</div>");
    }

$(function () {
@(for plots |
  var plot_@(div) = $.plot( $("#@(div)"),
    @(data),
    @(options)
 )
 @(textmarks)
|)
});
</script>
 </body>
</html>
