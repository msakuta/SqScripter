// Naive Squirrel object to JSON string converter, obtained from
// http://www.squirrel-lang.org/forums/default.aspx?g=posts&t=1872&p=2


function jsonize(obj)
/* return valid JSON string on success
   return null on failure (obj contains elements not convertable to JSON */
{
    local str ;
    local q = "\"";
    switch (typeof(obj))
    {
    case "string":
        return q + obj.tostring() + q;
       
    case "integer":
    case "float":
    case "bool":
        return obj.tostring();
       
    case "null":
        return "null";
       
    case "array":
        str = " [ ";
        foreach (element in obj)
        {
            local str2 = jsonize(element);
            if (str2 == null)
                return null;
            str += str2 + ", ";
       
        }
       
        str += "] ";
        return str;
       
    case "table":
        str = " { ";
        foreach (key, value in obj)
        {
            local key_string = jsonize(key);
            if (key_string == null)
                return null;
            local value_string = jsonize(value);
            if (value_string == null)
                return null;
            str += key_string + ": " + value_string + ",";
        }
        str += " } ";
        return str;

    default:
        return null;
    }
}
