(function(jQuery) {
	jQuery.fn.checkboxTree = function(settings) {

		settings = jQuery.extend({
			collapsedarrow: "../img/img-arrow-collapsed.gif",
			expandedarrow: "../img/img-arrow-expanded.gif",
			blankarrow: "../img/img-arrow-blank.gif",
			activeClass: "../img/checkboxtreeactive",
			hoverClass: "over",
			checkchildren: false,
			checkparents: false,
			collapsed: true,
			hidecbwithjs: true,
			checkedClass: "checked"
		}, settings);

		var $group = this;
		var $labels = jQuery(":checkbox + label",$group)
		var $checkboxes = jQuery(":checkbox",$group)

		//set up the tree
		jQuery("li",$group).each(function(){

			var $this = jQuery(this)
			var $collapseImage;

			if(settings.checkparents){
				var checkedchildren = jQuery("ul li input:checked",$this).length
				var $firstcheckbox = $this.children("input:checkbox:first")
				
				if((!$this.children("input:checkbox:first").is(":checked")) && (checkedchildren>0)){
					$firstcheckbox.attr("checked","checked")
				}
			}

			if($this.is(":has(ul)")){
				if(settings.collapsed){
					$this.find("ul").hide();
					$collapseImage = jQuery('<img class="checkboxtreeimage" src="' + settings.collapsedarrow + '" / >')
					$collapseImage.data("collapsedstate",0)
				}else{
					$collapseImage = jQuery('<img class="checkboxtreeimage" src="' + settings.expandedarrow + '" / >')
					$collapseImage.data("collapsedstate",1)
				}
			}else{
				$collapseImage = jQuery('<img src="' + settings.blankarrow + '" / >')
			}
			$this.prepend($collapseImage);
			
		})

		//add class signifying the tree has been setup
		$group.addClass(settings.activeClass);

		$group.bind("click.checkboxtree",function(e,a){
			var $clicked = jQuery(e.target);
			var $clickedcheck = $clicked.prev();
			var $currrow = $clicked.parents("li:first");
			var $clickedparent = $currrow.parents("li:first").find(":checkbox:first");
			var $clickedparentlabel = $clickedparent.next("label");
			var clickedlabel = false;
			var clickedimage = false;
			if($clicked.is(":checkbox + label")){
				clickedlabel=true;
			}
			if($clicked.is("img.checkboxtreeimage")){
				clickedimage=true;
			}
			//when the label is clicked, set the checkbox to the opposite clicked state, and toggle the checked class
			if(clickedlabel){                
                $clicked.prev().attr("checked", $clickedcheck.attr("checked")).end().toggleClass(settings.checkedClass);

                //check parents if that option is set
				if(settings.checkparents){
					var checkedchildren = jQuery("input:checked",$currrow).length
					
					if((!$clickedparent.is(":checked")) && (checkedchildren>0)){
						$clickedparentlabel.trigger("click.checkboxtree",["checkparents"])
					}
				}
				//check child checkboxes if settings say so
				if(settings.checkchildren && a !== "checkparents"){
                    var $test = $currrow.find(":checkbox + label").prev();
                    $test.slice(1).each(function() { $(this).attr({ "checked" : !$clickedcheck.attr("checked") }) })
                    $currrow.find(":checkbox + label").prev()
                    .next()
					.addClass($clicked.hasClass(settings.checkedClass)?settings.checkedClass:"")
					.removeClass($clicked.hasClass(settings.checkedClass)?"":settings.checkedClass)
				}
			}
			//handle expanding of levels
			if(clickedimage){
				var clickstate=$clicked.data("collapsedstate")
				if(clickstate===0){
					$currrow.children("ul").show();
					$currrow.children("img").attr("src",settings.expandedarrow);
					$clicked.data("collapsedstate",1)
				}else if(clickstate===1){
					$currrow.children("ul").hide();
					$currrow.children("img").attr("src",settings.collapsedarrow);
					$clicked.data("collapsedstate",0)
				}
			}
		})
		
		//set the class correctly for the labels that contain checked checkboxes
		jQuery("input:checked + label",$group).addClass(settings.checkedClass)

		//add hover class for labels if is ie 6
		if(jQuery.browser.msie&&jQuery.browser.version<7){
			$labels.hover( 
				function(){jQuery(this).addClass(settings.hoverClass)},
				function(){jQuery(this).removeClass(settings.hoverClass); }
			)
		}		

		//hide the checkboxes with js if set option set to true
		if(settings.hidecbwithjs){
			$checkboxes.hide();
		}

		
		return $group;
	};
})(jQuery);

