


typedef enum SMD_Manager_Result_e
{
	DOSEMANAGER_RESULT_UNDEFINED,
	DOSEMANAGER_RESULT_REQUEST,
	DOSEMANAGER_RESULT_START,
	DOSEMANAGER_RESULT_STOP,
	DOSEMANAGER_RESULT_RELEASE,
	DOSEMANAGER_RESULT_OPERATION_OK,
} SMD_Manager_Result_t;


typedef struct SMD_Manager_Callbacks_s
{
    void (*OperationCompleteCallback)(SMD_Manager_Result_t result);
    void (*ErrorCallback)(int errorCode);
    // Other callback function pointers...
} SMD_Manager_Callbacks_t;

typedef struct SMD_Manager_Configuration_s
{
    
    SMD_Manager_Callbacks_t callbacks;
    
} SMD_Manager_Configuration_t;