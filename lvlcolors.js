function () {

var prevonload = window.onload;
window.onload = function (e) {
	if (prevonload) prevonload(e);
    	
}

}()