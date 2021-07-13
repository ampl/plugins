/**********************************************************************
* FILENAME :        util.c
*
* DESCRIPTION :     Various useful functions
*
*                   rtrim  - right trim string
*                   getStr - get string from user
*                   getInt - get integer value from user
*                   extract_error - get error for handle from driver
*                   padOut - pad out string to given length
*                   itoa   - create string of integer
*                   getDataSources - get list of Data Sources
*                   selectDSN      - select data source
*                   getDescRecCount - get number of records in descriptor
*                   dumpDescriptorRec - uses SQLGetDescRec and output info
*                                       for descriptor record
*                   dumpDescriptorHeaderFields - outouts descriptor
*                                                header fields
*                   dumpDescriptorRecordFields - outputs descriptor
*                                                record fields
*                   dumpDescriptors - dumps out descriptors in use on handle
*                   hasBookmarkRecord - returns true/false to indicate
*                                       whether handle has bookmark rec (0)
*
* ODBC USAGE :
*                   See code.
*/


#include <cstring>




#define CHECK_ERROR(e, s, h, t) ({\
            if (e!=SQL_SUCCESS && e != SQL_SUCCESS_WITH_INFO) {extract_error(s, h, t); goto exit;} \
})

#define MAXDSNS 10
#define MAXDSNLEN 100

char* rtrim(char* string, char junk)
{
    char* original = string + strlen(string);
    while(*--original == junk);
        *(original + 1) = '\0';
    return string;
}

// get string from user.
char getStr (char *label, char *retStr, int len, char confirm) {

    char reply[3];
    char *nl=0;

    strcpy (reply, "Y");

    if (strlen(label) > 0) {
        printf ("%s : ", label);
    }

    fgets(retStr, len, stdin);
    if ( (nl = strchr (retStr, '\n')) != NULL) {*nl='\0';}

    if (confirm == 'Y') {
        printf ("Confirm Y/N? : ");
        fgets(reply, 3, stdin);
    }

    return reply[0];
}

// gets integer value from user. confirm ignored if number equals quitVal
char getInt (char *label, int *retInt, char confirm, int quitVal) {

    char reply[3];
    char buff[256];

    if (strlen(label)>0) {
        printf ("%s : ", label);
    }

    fgets(buff, sizeof(buff), stdin);
    *retInt=atoi(buff);
    if (confirm=='Y' && *retInt!=quitVal) {
        printf ("Confirm Y/N? : ");
        fgets(reply, 3, stdin);
    }

    return reply[0];
}

void extract_error(char *fn, SQLHANDLE handle, SQLSMALLINT type)
{
    SQLINTEGER i = 0;
    SQLINTEGER NativeError;
    SQLCHAR SQLState[ 7 ];
    SQLCHAR MessageText[256];
    SQLSMALLINT TextLength;
    SQLRETURN ret;

    fprintf(stderr, "\nThe driver reported the following error %s\n", fn);
    do
    {
        ret = SQLGetDiagRec(type, handle, ++i, SQLState, &NativeError,
                            MessageText, sizeof(MessageText), &TextLength);
        if (SQL_SUCCEEDED(ret)) {
            printf("%s:%ld:%ld:%s\n",
                        SQLState, (long) i, (long) NativeError, MessageText);
        }
    }
    while( ret == SQL_SUCCESS );
}

void padOut (char* data, char* padding, int max) {

    memset (padding, ' ', max);
    padding[max]='\0';

    //printf ("Data    '%s', Len %i\n", data, (int)strlen(data));
    //printf ("Padding '%s', Len %i\n", padding, (int)strlen(padding));
    //printf ("Max     %i\n", max);

    if (strlen(data)<strlen(padding)){
        padding[strlen(padding)-strlen(data)]='\0';
    } else {
        padding[0]='\0';
    }
    return;
}

//implemenation of itoa

char* itoa(int n) {
    char* ret = NULL;
    int numChars = 0;

    // Determine if integer is negative
    int isNegative = 0;

    if (n < 0) {
        n = -n;
        isNegative = 1;
        numChars++;
    }

    // Count how much space we will need for the string
    int temp = n;
    do {
        numChars++;
        temp /= 10;
    } while ( temp );

    // Allocate space for the string (1 for negative sign, 1 for each digit,
    // and 1 for null terminator)
    ret = malloc (numChars + 1);
    ret[numChars] = 0;

    // Add the negative sign if needed
    if (isNegative) ret[0] = '-';

    // Copy digits to string in reverse order
    int i = numChars - 1;
    do {
        ret[i--] = n%10 + '0';
        n /= 10;
    } while (n);

    return ret;
}

// Example of passing 2D array ...

int getDataSources (SQLHANDLE env, int maxNo, int maxLen,
                    char dsns[maxNo][maxLen]) {

	SQLRETURN retcode;
	SQLCHAR dsnName[256];
	SQLSMALLINT dsnNameLenReturned;
	SQLCHAR driverDesc[256];
	SQLSMALLINT driverDescLenReturned;
	SQLUSMALLINT direction;
    int count=0;

	direction = SQL_FETCH_FIRST;
	while (SQL_SUCCEEDED(retcode=SQLDataSources(env, direction, dsnName,
                                                sizeof(dsnName),
                                                &dsnNameLenReturned,
                                                driverDesc,
                                                sizeof(driverDesc),
                                                &driverDescLenReturned))) {
	    direction = SQL_FETCH_NEXT;
        strcpy (&dsns[count][0], dsnName);
        count++;
	}

    return count;
}

int selectDSN (SQLHANDLE henv, char *dsn, char *prompt) {

    char dsns[MAXDSNS][MAXDSNLEN];
    int count, i;
    count = getDataSources(henv, MAXDSNS, MAXDSNLEN, dsns);

    printf ("%s\n", prompt);
    printf ("0 - Quit\n");
    for (i=0;i<count;i++) {
        printf ("%i - %s\n", i+1, dsns[i]);
    }

    getInt ("DSN ? ", &i, 'N', 0);
    if (i==0)
        return SQL_ERROR;
    else {
        strcpy (dsn, &dsns[i-1][0]);
        return SQL_SUCCESS;
    }
}

//
// get number of column/param records in descriptor
// count does not include bookmark record ever
//
SQLSMALLINT getDescRecCount (SQLHDESC descriptor) {

    SQLRETURN retcode;
    SQLSMALLINT descCount=0;

    // get number of fields in the descriptor
    retcode = SQLGetDescField(descriptor, 0, SQL_DESC_COUNT, &descCount, 0, 0);
    if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
        extract_error("SQLGetDescField (COUNT)",
                                        descriptor, SQL_HANDLE_DESC);
        descCount=0;
    }
    return descCount;
}

//
// Dump various settings or values of multiple fields of a descriptor record
// using ODBC function SQLGetDescRec ()
//
void dumpDescriptorRec (char *comment,
                        SQLHDESC descriptor,
                        int incBookmarkRec) {

    int i;
    SQLRETURN       retcode;

    SQLCHAR         Name[255];
    SQLSMALLINT     BufferLength=255;
    SQLSMALLINT     StringLength;
    SQLSMALLINT     Type;
    SQLSMALLINT     SubType;
    SQLLEN          Length;
    SQLSMALLINT     Precision;
    SQLSMALLINT     Scale;
    SQLSMALLINT     Nullable;

    SQLSMALLINT     RecNumber;
    SQLSMALLINT     FieldIdentifier;

    SQLSMALLINT     descCount=0;
    SQLINTEGER      Len;

    printf ("\n---\n%s\n---\n", comment);
    descCount = getDescRecCount(descriptor);
    printf ("%i Records\n", (int) descCount);

    // Descriptor records are numbered from 0,
    // with record number 0 being the bookmark record.
    if (incBookmarkRec) {
        i=-1;
    } else {
        i=0;
    }

    for (;i<descCount;i++) {
        retcode = SQLGetDescRec(descriptor, i+1,
                                Name, BufferLength,
                                &StringLength, &Type,
                                &SubType, &Length,
                                &Precision, &Scale,
                                &Nullable);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            if (retcode==SQL_NO_DATA) {
                printf ("\nNo %s records !\n", comment);
            } else {
                 extract_error("SQLGetDescRec (hdbc)",
                                        descriptor, SQL_HANDLE_DESC);
                 break;
            }
        } else {
            printf ("\nName %s, ",      Name);
            printf ("Type %i, ",        (int)Type);
            printf ("SubType %i, ",     (int)SubType);
            printf ("Length %i, ",      (int)Length);
            printf ("Precision %i, ",   (int)Precision);
            printf (" Scale %i, ",      (int)Scale);
            printf ("Nullable %i\n",    (int)Nullable);
        }
    }

    return;
}

//
// Dumps out descriptor header record fields using ODBC
// function SQLGetDescField ()
//
void dumpDescriptorHeaderFields (SQLHDESC descriptor) {

    SQLRETURN       retcode;

    //
    // Header fields
    //
    SQLSMALLINT     descAllocType=0;
    SQLULEN         descArraySize=0;
    SQLUSMALLINT   *descArrayStatusPtr;
    SQLLEN         *descBindOffsetPtr;
    SQLINTEGER      descBindType=0;
    SQLSMALLINT     descCount=0;
    SQLULEN        *descRowsProcessesPtr;

    printf ("\n  Descriptor Header Fields");
    printf ("\n  ------------------------\n");
    retcode = SQLGetDescField(descriptor, 0, SQL_DESC_ALLOC_TYPE,
                                             &descAllocType, 0, 0);
    if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
        extract_error("SQLGetDescField (ALLOC_TYPE)",
                                descriptor, SQL_HANDLE_DESC);
        return;
    } else {
        printf (" SQL_DESC_ALLOC_TYPE : %i\n", (int) descAllocType);
    }

    retcode = SQLGetDescField(descriptor, 0, SQL_DESC_ARRAY_SIZE,
                                             &descArraySize, 0, 0);
    if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
        extract_error("SQLGetDescField (ARRAY_SIZE)",
                                descriptor, SQL_HANDLE_DESC);
        return;
    } else {
        printf (" SQL_DESC_ARRAY_SIZE : %i\n", (int) descArraySize);
    }

    retcode = SQLGetDescField(descriptor, 0, SQL_DESC_ARRAY_STATUS_PTR ,
                                             &descArrayStatusPtr, 0, 0);
    if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
        extract_error("SQLGetDescField (ARRAY_STATUS_PTR )",
                                            descriptor, SQL_HANDLE_DESC);
        return;
    } else {
        printf (" SQL_DESC_ARRAY_STATUS_PTR  : %p\n", descArrayStatusPtr);
    }

    retcode = SQLGetDescField(descriptor, 0, SQL_DESC_BIND_OFFSET_PTR ,
                                             &descBindOffsetPtr, 0, 0);
    if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
        extract_error("SQLGetDescField (BIND_OFFSET_PTR )",
                                            descriptor, SQL_HANDLE_DESC);
        return;
    } else {
        printf (" SQL_DESC_BIND_OFFSET_PTR  : %p\n", descBindOffsetPtr);
    }

    retcode = SQLGetDescField(descriptor, 0, SQL_DESC_BIND_TYPE ,
                                             &descBindType, 0, 0);
    if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
        extract_error("SQLGetDescField (BIND_TYPE )",
                                            descriptor, SQL_HANDLE_DESC);
        return;
    } else {
        printf (" SQL_DESC_BIND_TYPE  : %i\n", descBindType);
    }

    retcode = SQLGetDescField(descriptor, 0, SQL_DESC_COUNT ,
                                             &descCount, 0, 0);
    if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
        extract_error("SQLGetDescField (COUNT )",
                                            descriptor, SQL_HANDLE_DESC);
        return;
    } else {
        printf (" SQL_DESC_COUNT  : %i\n", descCount);
    }

    retcode = SQLGetDescField(descriptor, 0, SQL_DESC_ROWS_PROCESSED_PTR ,
                                             &descRowsProcessesPtr, 0, 0);
    if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
        extract_error("SQLGetDescField (ROWS_PROCESSED_PTR )",
                                            descriptor, SQL_HANDLE_DESC);
        return;
    } else {
        printf (" SQL_DESC_ROWS_PROCESSED_PTR  : %p\n", descRowsProcessesPtr);
    }

    return;
}

//
// Dump out descriptor records fields using ODBC function SQLGetDescField ()
//
void dumpDescriptorRecordFields (SQLHDESC descriptor, int incBookmarkRec) {

    SQLINTEGER      descAutoUniqueValue;
    SQLCHAR         descBaseColumnName[255];
    SQLCHAR         descBaseTableName[255];
    SQLINTEGER      descCaseSensitive;
    SQLCHAR         descCatalogName[255];
    SQLSMALLINT     descConsiseType;
    SQLPOINTER      descDataPtr;
    SQLSMALLINT     descDatetimeIntervalCode;
    SQLINTEGER      descDatetimeIntervalPrecision;
    SQLLEN          descDisplaySize;
    SQLSMALLINT     descFixedPreCScale;
    SQLLEN         *descIndicatorPtr;
    SQLCHAR         descLabel[255];
    SQLULEN         descLength;
    SQLCHAR         descLiteralPrefix[255];
    SQLCHAR         descLiteralSuffix[255];
    SQLCHAR         descLocalTypeName[255];
    SQLCHAR         descName[255];
    SQLSMALLINT     descNullable;
    SQLINTEGER      descNumPrecRadix;
    SQLLEN          descOctetLength;
    SQLLEN         *descOctetLengthPtr;
    SQLSMALLINT     descParameterType;
    SQLSMALLINT     descPrecision;
    SQLSMALLINT     descRowver;
    SQLSMALLINT     descScale;
    SQLCHAR         descSchemaName[255];
    SQLSMALLINT     descSearchable;
    SQLCHAR         descTableName[255];
    SQLSMALLINT     descType=0;
    SQLCHAR         descTypeName[255];
    SQLSMALLINT     descUnnamed;
    SQLSMALLINT     descUnsigned;
    SQLSMALLINT     descUpdatable;
    SQLINTEGER      Len;

    SQLRETURN retcode;
    SQLSMALLINT descCount;

    descCount=getDescRecCount (descriptor);

    printf ("\n  Descriptor Record Fields");
    printf ("\n  ------------------------\n");

    int i;
    if (incBookmarkRec) {
        i=-1;
    } else {
        i=0;
    }
    for (;i<descCount;i++) {
        printf ("\n  Record %i ", i+1);
        if (i==-1) printf ("(BOOKMARK RECORD)");
        printf ("\n");
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_AUTO_UNIQUE_VALUE ,
                                  &descAutoUniqueValue, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (AUTO_UNIQUE_VALUE)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_AUTO_UNIQUE_VALUE  : %i\n", descAutoUniqueValue);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_BASE_COLUMN_NAME ,
                                  descBaseColumnName, 255, &Len);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (BASE_COLUMN_NAME)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_BASE_COLUMN_NAME  : '%s'\n",
                                                descBaseColumnName);
        }

        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_BASE_TABLE_NAME ,
                                  descBaseTableName, 255, &Len);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (BASE_TABLE_NAME)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_BASE_TABLE_NAME  : '%s'\n", descBaseTableName);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_CASE_SENSITIVE ,
                                  &descCaseSensitive, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (CASE_SENSITIVE)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_CASE_SENSITIVE  : %i\n", descCaseSensitive);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_CATALOG_NAME ,
                                  &descCatalogName, 255, &Len);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (CATALOG_NAME)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_CATALOG_NAME  : '%s'\n", descCatalogName);
        }

        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_CONCISE_TYPE ,
                                  &descConsiseType, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (CONCISE_TYPE)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_CONCISE_TYPE  : %i\n", descConsiseType);
        }

        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_DATA_PTR ,
                                  &descDataPtr, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (DATA_PTR)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_DATA_PTR  : %p\n", descDataPtr);
        }

        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_DATETIME_INTERVAL_CODE ,
                                  &descDatetimeIntervalCode, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (DATETIME_INTERVAL_CODE)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_DATETIME_INTERVAL_CODE  : %i\n",
                                                (int)descDatetimeIntervalCode);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_DATETIME_INTERVAL_PRECISION ,
                                  &descDatetimeIntervalPrecision, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (DATETIME_INTERVAL_PRECISION)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_DATETIME_INTERVAL_PRECISION  : %i\n",
                                                descDatetimeIntervalPrecision);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_DISPLAY_SIZE ,
                                  &descDisplaySize, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (DISPLAY_SIZE)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_DISPLAY_SIZE  : %i\n", (int)descDisplaySize);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_FIXED_PREC_SCALE ,
                                  &descFixedPreCScale, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (FIXED_PREC_SCALE)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_FIXED_PREC_SCALE  : %i\n",
                                                (int)descFixedPreCScale);
        }

        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_INDICATOR_PTR ,
                                  &descIndicatorPtr, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (INDICATOR_PTR)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_INDICATOR_PTR  : %p\n", descIndicatorPtr);
        }

        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_LABEL ,
                                  descLabel, 255, &Len);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (LABEL )",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_LABEL  : '%s'\n", descLabel);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_LENGTH ,
                                  &descLength, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (LENGTH)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_LENGTH  : %i\n", (unsigned int)descLength);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_LITERAL_PREFIX ,
                                  descLiteralPrefix, 255, &Len);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (LITERAL_PREFIX)",
                                                descriptor, SQL_HANDLE_DESC);

        } else {
            printf (" SQL_DESC_LITERAL_PREFIX  : '%s'\n", descLiteralPrefix);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_LITERAL_SUFFIX ,
                                  descLiteralSuffix, 255, &Len);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (LITERAL_SUFFIX)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_LITERAL_SUFFIX  : '%s'\n", descLiteralSuffix);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_LOCAL_TYPE_NAME ,
                                  descLocalTypeName, 255, &Len);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (LOCAL_TYPE_NAME)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_LOCAL_TYPE_NAME  : '%s'\n", descLocalTypeName);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_NAME ,
                                  descName, 255, &Len);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (NAME )",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_NAME  : '%s'\n", descName);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_NULLABLE ,
                                  &descNullable, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (NULLABLE)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_NULLABLE  : %i\n", (int)descNullable);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_NUM_PREC_RADIX ,
                                  &descNumPrecRadix, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (NUM_PREC_RADIX)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_NUM_PREC_RADIX  : %i\n", (int)descNumPrecRadix);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_OCTET_LENGTH ,
                                  &descOctetLength, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (OCTET_LENGTH)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_OCTET_LENGTH  : %i\n", (int)descOctetLength);
        }

        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_OCTET_LENGTH_PTR ,
                                  &descOctetLengthPtr, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (OCTET_LENGTH_PTR)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_OCTET_LENGTH_PTR  : %p\n", descOctetLengthPtr);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_PARAMETER_TYPE ,
                                  &descParameterType, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (PARAMETER_TYPE)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_PARAMETER_TYPE  : %i\n", (int)descParameterType);
        }

        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_PRECISION ,
                                  &descPrecision, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (PRECISION)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_PRECISION  : %i\n", (int)descPrecision);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_ROWVER ,
                                  &descRowver, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (ROWVER)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_ROWVER  : %i\n", (int)descRowver);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_SCALE ,
                                  &descScale, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (SCALE)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_SCALE  : %i\n", (int)descScale);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_SCHEMA_NAME ,
                                  &descSchemaName, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (SCHEMA_NAME)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_SCHEMA_NAME  : '%s'\n", descSchemaName);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_SEARCHABLE ,
                                  &descSearchable, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (SEARCHABLE)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_SEARCHABLE  : %i\n", (int)descSearchable);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_TYPE ,
                                  &descType, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (TYPE )",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_TYPE  : %i\n", (int)descType);
        }

        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_TYPE_NAME ,
                                  descTypeName, 255, &Len);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (TYPE_NAME )",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_TYPE_NAME  : '%s'\n", descTypeName);
        }

        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_UNNAMED ,
                                  &descUnnamed, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (UNNAMED)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_UNNAMED  : %i\n", descUnnamed);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_UNSIGNED ,
                                  &descUnsigned, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (UNSIGNED)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_UNSIGNED  : %i\n", descUnsigned);
        }
         retcode = SQLGetDescField(descriptor, i+1,
                                   SQL_DESC_UPDATABLE ,
                                   &descUpdatable, 0, 0);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (UPDATABLE)",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_UPDATABLE  : %i\n", descUpdatable);
        }
        retcode = SQLGetDescField(descriptor, i+1,
                                  SQL_DESC_TABLE_NAME ,
                                  descTableName, 255, &Len);
        if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
            extract_error("SQLGetDescField (TABLE_NAME )",
                                                descriptor, SQL_HANDLE_DESC);
        } else {
            printf (" SQL_DESC_TABLE_NAME  : '%s'\n", descTableName);
        }

    }

    return;
}

//
// dump descriptors
//
void dumpDescriptors (SQLCHAR *comment,         // display heading/comment
                      SQLHSTMT hstmt,           // statement handle
                      USHORT header,            // display header record
                      USHORT records,           // display col/parm records
                      int incBookmarkRec) {     // include bookmark record

    SQLRETURN retcode;
    SQLHDESC  hArd, hIrd, hApd, hIpd;           // descriptors

    // Get ARD
    retcode = SQLGetStmtAttr(hstmt, SQL_ATTR_APP_ROW_DESC, &hArd, 0, NULL);
    CHECK_ERROR(retcode, "SQLGetStmtAttr(SQL_HANDLE_STMT)",
                hstmt, SQL_HANDLE_STMT);

    // Get IRD
    retcode = SQLGetStmtAttr(hstmt, SQL_ATTR_IMP_ROW_DESC, &hIrd, 0, NULL);
    CHECK_ERROR(retcode, "SQLExecDirect(SQL_HANDLE_STMT)",
                hstmt, SQL_HANDLE_STMT);

    // Get APD
    retcode = SQLGetStmtAttr(hstmt, SQL_ATTR_APP_PARAM_DESC, &hApd, 0, NULL);
    CHECK_ERROR(retcode, "SQLExecDirect(SQL_HANDLE_STMT)",
                hstmt, SQL_HANDLE_STMT);

    // Get IPD
    retcode = SQLGetStmtAttr(hstmt, SQL_ATTR_IMP_PARAM_DESC, &hIpd, 0, NULL);
    CHECK_ERROR(retcode, "SQLExecDirect(SQL_HANDLE_STMT)",
                hstmt, SQL_HANDLE_STMT);

    printf ("\n---\n%s\n---\n", comment);
    dumpDescriptorRec ("ARD", hArd, incBookmarkRec);
    if (header=='Y') {
        dumpDescriptorHeaderFields (hArd);
    }
    if (records=='Y') {
        dumpDescriptorRecordFields (hArd, incBookmarkRec);
    }

    dumpDescriptorRec ("IRD", hIrd, incBookmarkRec);
    if (header=='Y') {
        dumpDescriptorHeaderFields (hIrd);
    }
    if (records='Y') {
        dumpDescriptorRecordFields (hIrd, incBookmarkRec);
    }

    dumpDescriptorRec ("APD", hApd, incBookmarkRec);
    if (header=='Y') {
        dumpDescriptorHeaderFields (hApd);
    }
    if (records='Y') {
        dumpDescriptorRecordFields (hApd, incBookmarkRec);
    }

    dumpDescriptorRec ("IPD", hIpd, incBookmarkRec);
    if (header=='Y') {
        dumpDescriptorHeaderFields (hIpd);
    }
    if (records='Y') {
        dumpDescriptorRecordFields (hIpd, incBookmarkRec);
    }

exit:

    return;
}

//
// returns true/false if descriptor has/hasn't bookmark record
//
int hasBookmarkRecord (SQLHANDLE handle) {

    SQLRETURN retcode;
    SQLULEN   bookmark;

    retcode = SQLGetStmtAttr(handle, SQL_ATTR_USE_BOOKMARKS,
                             &bookmark, 0, 0);
    if ( (retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO) ) {
        extract_error("SQLGetStmtAttr (SQL_ATTR_USE_BOOKMARKS)",
                      handle, SQL_HANDLE_ENV);
        return 0;
    }
    if (bookmark==SQL_UB_OFF)
        return 0;
    else
        return 1;
}
