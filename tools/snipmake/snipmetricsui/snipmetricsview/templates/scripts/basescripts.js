
function switchFieldVisibility(fieldName)
{
      field = document.getElementById(fieldName);
      field_label = field.previousSibling.previousSibling;
      if (field.style.display == 'none')
      {
         field.style.display = 'inline';
         field_label.style.display = 'inline';
      }
      else
      {
        field.style.display = 'none';
        field_label.style.display = 'none';
      }
}