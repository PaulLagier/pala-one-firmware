#ifndef PALA_LANG_ES_LA_H
#define PALA_LANG_ES_LA_H

// ============================================================================
//  Spanish (Latin America) string table — es_LA.
//  Mirror of en.h; the key set MUST stay identical. Adding new keys: edit en.h
//  first, then add the same key here. See src/lang/lang.h for the rule.
//
//  Glyph coverage: all accents used here (á é í ó ú ñ ¿ ¡ ü) are in u8g2's
//  Latin Extended (_te) font set already linked by src/ui/font.cpp, so no
//  font change is required. Web responses already declare charset=utf-8.
// ============================================================================

// ----------------------------------------------------------------------------
//  Boot / fatal screens
// ----------------------------------------------------------------------------
#undef D_BOOT_STORAGE_ERROR
#define D_BOOT_STORAGE_ERROR        "Error de almacenamiento"
#undef D_BOOT_TRY_FACTORY_RESET
#define D_BOOT_TRY_FACTORY_RESET    "Pruebe reinicio de fábrica"

// ----------------------------------------------------------------------------
//  About screen
// ----------------------------------------------------------------------------
#undef D_ABOUT_HEADER
#define D_ABOUT_HEADER              "Dispositivo"
#undef D_ABOUT_FIRMWARE_PREFIX
#define D_ABOUT_FIRMWARE_PREFIX     "Firmware "
#undef D_ABOUT_GESTURE_NEXT
#define D_ABOUT_GESTURE_NEXT        "1x siguiente / abajo"
#undef D_ABOUT_GESTURE_OPEN
#define D_ABOUT_GESTURE_OPEN        "2x abrir / elegir"
#undef D_ABOUT_GESTURE_HOME
#define D_ABOUT_GESTURE_HOME        "3x inicio"
#undef D_ABOUT_GESTURE_BOOKMARK
#define D_ABOUT_GESTURE_BOOKMARK    "Mantener: marcapáginas"

// ----------------------------------------------------------------------------
//  Library menu entries
// ----------------------------------------------------------------------------
#undef D_MENU_BOOKMARKS
#define D_MENU_BOOKMARKS            "Marcapáginas"
#undef D_MENU_LIST
#define D_MENU_LIST                 "Lista"
#undef D_MENU_APPS
#define D_MENU_APPS                 "Apps"
#undef D_MENU_STATISTICS
#define D_MENU_STATISTICS           "Estadísticas"
#undef D_MENU_DEVICE
#define D_MENU_DEVICE               "Dispositivo"
#undef D_MENU_UPLOAD
#define D_MENU_UPLOAD               "Conectar"
#undef D_LIBRARY_OPEN_FAILED
#define D_LIBRARY_OPEN_FAILED       "Error al abrir"
#undef D_LIBRARY_TRY_UPLOAD
#define D_LIBRARY_TRY_UPLOAD        "Intente subir de nuevo"

// ----------------------------------------------------------------------------
//  Statistics screen
// ----------------------------------------------------------------------------
#undef D_STATS_HEADING
#define D_STATS_HEADING                "Estadísticas"
#undef D_STATS_STREAK_CURRENT_FMT
#define D_STATS_STREAK_CURRENT_FMT     "Racha actual: %u días"
#undef D_STATS_STREAK_LONGEST_FMT
#define D_STATS_STREAK_LONGEST_FMT     "Mejor: %u  Sesiones: %u"
#undef D_STATS_LIFETIME_PAGES_FMT
#define D_STATS_LIFETIME_PAGES_FMT     "Páginas leídas: %llu"
#undef D_STATS_LIFETIME_PRESSES_FMT
#define D_STATS_LIFETIME_PRESSES_FMT   "Pulsaciones: %llu"

// ----------------------------------------------------------------------------
//  List screen
// ----------------------------------------------------------------------------
#undef D_LIST_HEADER
#define D_LIST_HEADER               "Lista"
#undef D_LIST_NONE
#define D_LIST_NONE                 "Sin elementos"

// ----------------------------------------------------------------------------
//  Upload screen
// ----------------------------------------------------------------------------
#undef D_UPLOAD_HEADER
#define D_UPLOAD_HEADER             "Subir"
#undef D_UPLOAD_WIFI
#define D_UPLOAD_WIFI               "Wi-Fi"
#undef D_UPLOAD_PASSWORD
#define D_UPLOAD_PASSWORD           "Contraseña"
#undef D_UPLOAD_OPEN
#define D_UPLOAD_OPEN               "Abrir"
#undef D_UPLOAD_CONNECTING
#define D_UPLOAD_CONNECTING         "Conectando"
#undef D_UPLOAD_CONNECTED
#define D_UPLOAD_CONNECTED          "Conectado"
#undef D_UPLOAD_HOTSPOT_HINT_L1
#define D_UPLOAD_HOTSPOT_HINT_L1    "Pulsa el botón para"
#undef D_UPLOAD_HOTSPOT_HINT_L2
#define D_UPLOAD_HOTSPOT_HINT_L2    "usar punto de acceso"

// ----------------------------------------------------------------------------
//  Apps screen
// ----------------------------------------------------------------------------
#undef D_APPS_HEADER
#define D_APPS_HEADER               "Apps"
#undef D_APPS_NONE
#define D_APPS_NONE                 "Sin apps"

// ----------------------------------------------------------------------------
//  Bookmarks screens
// ----------------------------------------------------------------------------
#undef D_BOOKMARKS_HEADER
#define D_BOOKMARKS_HEADER          "Marcapáginas"
#undef D_BOOKMARKS_NO_BOOKS
#define D_BOOKMARKS_NO_BOOKS        "Sin libros"
#undef D_BOOKMARKS_NONE
#define D_BOOKMARKS_NONE            "Sin marcapáginas"
#undef D_BOOKMARKS_OPEN_FAILED
#define D_BOOKMARKS_OPEN_FAILED     "Error al abrir"

// ----------------------------------------------------------------------------
//  Reader
// ----------------------------------------------------------------------------
#undef D_READER_BOOK_EMPTY
#define D_READER_BOOK_EMPTY         "Libro vacío"
#undef D_READER_BACK_LIBRARY
#define D_READER_BACK_LIBRARY       "Volver a biblioteca"

// ----------------------------------------------------------------------------
//  App loader error overlay
// ----------------------------------------------------------------------------
#undef D_APP_ERR_TITLE
#define D_APP_ERR_TITLE             "Error de app"
#undef D_APP_ERR_NULL_PATH
#define D_APP_ERR_NULL_PATH         "ruta nula"
#undef D_APP_ERR_NOT_FOUND
#define D_APP_ERR_NOT_FOUND         "App no encontrada"
#undef D_APP_ERR_TOO_SMALL
#define D_APP_ERR_TOO_SMALL         "App muy pequeña"
#undef D_APP_ERR_INVALID_FILE
#define D_APP_ERR_INVALID_FILE      "Archivo inválido"
#undef D_APP_ERR_TOO_LARGE
#define D_APP_ERR_TOO_LARGE         "App muy grande"
#undef D_APP_ERR_SIZE_LIMIT
#define D_APP_ERR_SIZE_LIMIT        "> 48 KB"
#undef D_APP_ERR_READ
#define D_APP_ERR_READ              "Error de lectura"
#undef D_APP_ERR_PARTIAL_READ
#define D_APP_ERR_PARTIAL_READ      "Lectura parcial"
#undef D_APP_ERR_NO_EXEC_MEM
#define D_APP_ERR_NO_EXEC_MEM       "Sin memoria ejec."
#undef D_APP_ERR_BAD_FILE
#define D_APP_ERR_BAD_FILE          "App inválida"
#undef D_APP_ERR_WRONG_MAGIC
#define D_APP_ERR_WRONG_MAGIC       "Firma incorrecta"
#undef D_APP_ERR_API_MISMATCH
#define D_APP_ERR_API_MISMATCH      "API incompatible"
#undef D_APP_ERR_API_FMT
#define D_APP_ERR_API_FMT           "API v%u, requiere v%u"
#undef D_APP_ERR_BAD_ENTRY
#define D_APP_ERR_BAD_ENTRY         "Entrada inválida"
#undef D_APP_ERR_BAD_RELOC
#define D_APP_ERR_BAD_RELOC         "Tabla reloc inválida"
#undef D_APP_ERR_RELOC_RANGE
#define D_APP_ERR_RELOC_RANGE       "Reloc fuera de rango"

// ----------------------------------------------------------------------------
//  Bookmark add toasts
// ----------------------------------------------------------------------------
#undef D_TOAST_BOOKMARK_EXISTS
#define D_TOAST_BOOKMARK_EXISTS     "Marcapáginas ya existe"
#undef D_TOAST_BOOKMARK_SAVED
#define D_TOAST_BOOKMARK_SAVED      "Marcapáginas guardado"

// ----------------------------------------------------------------------------
//  Lock / screensaver
// ----------------------------------------------------------------------------
#undef D_SCREENSAVER_LOCKED
#define D_SCREENSAVER_LOCKED        "Bloqueado"
#undef D_TOAST_UNLOCKED
#define D_TOAST_UNLOCKED            "Desbloqueado"

// ============================================================================
//  Web UI
// ============================================================================

// ----------------------------------------------------------------------------
//  Shared chrome / storage card
// ----------------------------------------------------------------------------
#undef D_WEB_STORAGE_HEADING
#define D_WEB_STORAGE_HEADING       "Almacenamiento"
#undef D_WEB_STORAGE_BOOKS
#define D_WEB_STORAGE_BOOKS         "Libros"
#undef D_WEB_STORAGE_USED
#define D_WEB_STORAGE_USED          "Usado"
#undef D_WEB_STORAGE_FREE
#define D_WEB_STORAGE_FREE          "Libre"
#undef D_WEB_STORAGE_TOTAL
#define D_WEB_STORAGE_TOTAL         "Total"
#undef D_WEB_STORAGE_PCT_SUFFIX
#define D_WEB_STORAGE_PCT_SUFFIX    "% del almacenamiento interno en uso."

// ----------------------------------------------------------------------------
//  Navigation links
// ----------------------------------------------------------------------------
#undef D_WEB_NAV_HOME
#define D_WEB_NAV_HOME              "Inicio"
#undef D_WEB_NAV_FILES
#define D_WEB_NAV_FILES             "Archivos"
#undef D_WEB_NAV_BOOKMARKS
#define D_WEB_NAV_BOOKMARKS         "Marcapáginas"
#undef D_WEB_NAV_LIST
#define D_WEB_NAV_LIST              "Lista"
#undef D_WEB_NAV_SETTINGS
#define D_WEB_NAV_SETTINGS          "Ajustes"
#undef D_WEB_NAV_FACTORY_RESET
#define D_WEB_NAV_FACTORY_RESET     "Reinicio de fábrica"
#undef D_WEB_NAV_BACK
#define D_WEB_NAV_BACK              "Atrás"

// ----------------------------------------------------------------------------
//  Home page
// ----------------------------------------------------------------------------
#undef D_WEB_HOME_TITLE
#define D_WEB_HOME_TITLE            "Pala One"
#undef D_WEB_HOME_FW_PREFIX
#define D_WEB_HOME_FW_PREFIX        "Firmware "
#undef D_WEB_HOME_MIDDOT_SEP
#define D_WEB_HOME_MIDDOT_SEP       " &middot; "
#undef D_WEB_HOME_BOOKS_SUFFIX
#define D_WEB_HOME_BOOKS_SUFFIX     " libros"
#undef D_WEB_HOME_FREE_LABEL
#define D_WEB_HOME_FREE_LABEL       "Libre: "
#undef D_WEB_HOME_STORAGE_WARN
#define D_WEB_HOME_STORAGE_WARN     "&#9888; Almacenamiento no disponible o casi lleno. Si las subidas fallan, elimine libros o use Reinicio de fábrica desde esta interfaz web."
#undef D_WEB_UPLOAD_BOOK_HEADING
#define D_WEB_UPLOAD_BOOK_HEADING   "Subir libro"
#undef D_WEB_UPLOAD_BOOK_DESC
#define D_WEB_UPLOAD_BOOK_DESC      "Envíe archivos UTF-8 de texto plano a <b>/books</b> en el dispositivo, luego organícelos en carpetas desde la página Archivos."
#undef D_WEB_UPLOAD_BOOK_BUTTON
#define D_WEB_UPLOAD_BOOK_BUTTON    "Subir"
#undef D_WEB_MANAGE_FILES_BUTTON
#define D_WEB_MANAGE_FILES_BUTTON   "Administrar archivos"
#undef D_WEB_INSTALL_APP_HEADING
#define D_WEB_INSTALL_APP_HEADING   "Instalar app"
#undef D_WEB_INSTALL_APP_DESC
#define D_WEB_INSTALL_APP_DESC      "Suba un binario de app Pala (<b>.bin</b>) a <b>/apps</b>. El encabezado se valida antes de confirmar; solo se aceptan archivos con la firma y versión de API correctas. Abra <b>Apps</b> desde la biblioteca para ejecutarla."
#undef D_WEB_INSTALL_APP_BUTTON
#define D_WEB_INSTALL_APP_BUTTON    "Instalar app"
#undef D_WEB_NOTES_HEADING
#define D_WEB_NOTES_HEADING         "Notas"
#undef D_WEB_NOTES_DESC
#define D_WEB_NOTES_DESC            "Los libros subidos se normalizan y compactan antes de guardarse, por lo que un TXT de origen puede ser más grande que el archivo final almacenado. El lector está optimizado para texto plano UTF-8 e idiomas con alfabeto latino."

// ----------------------------------------------------------------------------
//  Files page
// ----------------------------------------------------------------------------
#undef D_WEB_FILES_HEADING
#define D_WEB_FILES_HEADING         "Archivos"
#undef D_WEB_FILES_SUBTITLE
#define D_WEB_FILES_SUBTITLE        "Administre libros, carpetas y estructura de la biblioteca de Pala One."
#undef D_WEB_CREATE_FOLDER_HEADING
#define D_WEB_CREATE_FOLDER_HEADING "Crear carpeta"
#undef D_WEB_CREATE_FOLDER_PLACEHOLDER
#define D_WEB_CREATE_FOLDER_PLACEHOLDER "libros o clasicos/espanol"
#undef D_WEB_CREATE_FOLDER_BUTTON
#define D_WEB_CREATE_FOLDER_BUTTON  "Crear carpeta"
#undef D_WEB_CREATE_FOLDER_HINT
#define D_WEB_CREATE_FOLDER_HINT    "Las carpetas viven dentro de /books."
#undef D_WEB_FOLDERS_HEADING
#define D_WEB_FOLDERS_HEADING       "Carpetas"
#undef D_WEB_NO_FOLDERS
#define D_WEB_NO_FOLDERS            "Aún no hay carpetas. Los libros viven en la raíz de /books."
#undef D_WEB_CONFIRM_DELETE_FOLDER
#define D_WEB_CONFIRM_DELETE_FOLDER "¿Eliminar carpeta? Solo se pueden eliminar carpetas vacías."
#undef D_WEB_DELETE_BUTTON
#define D_WEB_DELETE_BUTTON         "Eliminar"
#undef D_WEB_LIBRARY_FILES_HEADING
#define D_WEB_LIBRARY_FILES_HEADING "Archivos de biblioteca"
#undef D_WEB_LIBRARY_FULL_WARN
#define D_WEB_LIBRARY_FULL_WARN     "&#9888; Biblioteca llena (máx. 80 libros). Elimine libros para hacer espacio."
#undef D_WEB_FOLDER_LIMIT_WARN
#define D_WEB_FOLDER_LIMIT_WARN     "&#9888; Límite de carpetas alcanzado (máx. 32)."
#undef D_WEB_NO_BOOKS_UPLOADED
#define D_WEB_NO_BOOKS_UPLOADED     "Aún no se han subido libros."
#undef D_WEB_BOOK_ROOT
#define D_WEB_BOOK_ROOT             "Raíz"
#undef D_WEB_BOOK_BYTES_LABEL
#define D_WEB_BOOK_BYTES_LABEL      " bytes"
#undef D_WEB_BOOK_FOLDER_LABEL
#define D_WEB_BOOK_FOLDER_LABEL     " &middot; carpeta: "
#undef D_WEB_BOOK_CURRENT_PAGE
#define D_WEB_BOOK_CURRENT_PAGE     " &middot; página actual: "
#undef D_WEB_JUMP_BUTTON
#define D_WEB_JUMP_BUTTON           "Ir"
#undef D_WEB_JUMP_HINT
#define D_WEB_JUMP_HINT             "Establezca la página que se abrirá la próxima vez en el dispositivo."
#undef D_WEB_JUMP_HINT2
#define D_WEB_JUMP_HINT2            "La primera apertura puede tardar un momento."
#undef D_WEB_PAGE_PLACEHOLDER
#define D_WEB_PAGE_PLACEHOLDER      "Página"
#undef D_WEB_MOVE_BUTTON
#define D_WEB_MOVE_BUTTON           "Mover"
#undef D_WEB_MOVE_HINT
#define D_WEB_MOVE_HINT             "Use la ruta exacta de la carpeta."
#undef D_WEB_MOVE_PLACEHOLDER
#define D_WEB_MOVE_PLACEHOLDER      "vacío para raíz"
#undef D_WEB_CONFIRM_DELETE_FILE
#define D_WEB_CONFIRM_DELETE_FILE   "¿Eliminar archivo?"
#undef D_WEB_APPS_PAGE_HEADING
#define D_WEB_APPS_PAGE_HEADING     "Apps"
#undef D_WEB_NO_APPS_INSTALLED
#define D_WEB_NO_APPS_INSTALLED     "Sin apps instaladas."
#undef D_WEB_CONFIRM_DELETE_APP
#define D_WEB_CONFIRM_DELETE_APP    "¿Eliminar app?"

// ----------------------------------------------------------------------------
//  Plain-text 4xx/5xx error bodies
// ----------------------------------------------------------------------------
#undef D_WEB_ERR_MISSING_ID
#define D_WEB_ERR_MISSING_ID            "id faltante"
#undef D_WEB_ERR_BAD_ID
#define D_WEB_ERR_BAD_ID                "id inválido"
#undef D_WEB_ERR_MISSING_FOLDER
#define D_WEB_ERR_MISSING_FOLDER        "carpeta faltante"
#undef D_WEB_ERR_BAD_FOLDER
#define D_WEB_ERR_BAD_FOLDER            "carpeta inválida"
#undef D_WEB_ERR_FOLDER_LIMIT
#define D_WEB_ERR_FOLDER_LIMIT          "límite de carpetas alcanzado"
#undef D_WEB_ERR_MKDIR_FAILED
#define D_WEB_ERR_MKDIR_FAILED          "fallo al crear carpeta"
#undef D_WEB_ERR_FOLDER_NOT_FOUND
#define D_WEB_ERR_FOLDER_NOT_FOUND      "carpeta no encontrada"
#undef D_WEB_ERR_FOLDER_NOT_EMPTY
#define D_WEB_ERR_FOLDER_NOT_EMPTY      "carpeta no vacía"
#undef D_WEB_ERR_DELETE_FAILED
#define D_WEB_ERR_DELETE_FAILED         "fallo al eliminar"
#undef D_WEB_ERR_FOLDER_CREATE_FAILED
#define D_WEB_ERR_FOLDER_CREATE_FAILED  "fallo al crear carpeta"
#undef D_WEB_ERR_DEST_EXISTS
#define D_WEB_ERR_DEST_EXISTS           "destino ya existe"
#undef D_WEB_ERR_MOVE_FAILED
#define D_WEB_ERR_MOVE_FAILED           "fallo al mover"
#undef D_WEB_ERR_MISSING_ID_PAGE
#define D_WEB_ERR_MISSING_ID_PAGE       "id/página faltante"
#undef D_WEB_ERR_MISSING_BOOK_IDX
#define D_WEB_ERR_MISSING_BOOK_IDX      "libro/idx faltante"
#undef D_WEB_ERR_BAD_BOOK
#define D_WEB_ERR_BAD_BOOK              "libro inválido"
#undef D_WEB_ERR_BAD_IDX
#define D_WEB_ERR_BAD_IDX               "idx inválido"
#undef D_WEB_ERR_MISSING_BOOK
#define D_WEB_ERR_MISSING_BOOK          "libro faltante"
#undef D_WEB_ERR_MISSING_NAME
#define D_WEB_ERR_MISSING_NAME          "nombre faltante"
#undef D_WEB_ERR_INVALID_NAME
#define D_WEB_ERR_INVALID_NAME          "nombre inválido"

// ----------------------------------------------------------------------------
//  List page
// ----------------------------------------------------------------------------
#undef D_WEB_LIST_HEADING
#define D_WEB_LIST_HEADING          "Lista"
#undef D_WEB_LIST_SUBTITLE
#define D_WEB_LIST_SUBTITLE         "Cree una lista simple de compras o tareas para Pala One."
#undef D_WEB_LIST_EDIT_HEADING
#define D_WEB_LIST_EDIT_HEADING     "Editar lista"
#undef D_WEB_LIST_EDIT_DESC
#define D_WEB_LIST_EDIT_DESC        "Los elementos aparecen en el dispositivo solo cuando al menos una línea contiene texto. Mantenga el botón en el dispositivo para marcar un elemento como hecho."
#undef D_WEB_LIST_ITEM_PLACEHOLDER
#define D_WEB_LIST_ITEM_PLACEHOLDER "Elemento de lista"
#undef D_WEB_LIST_SAVE_BUTTON
#define D_WEB_LIST_SAVE_BUTTON      "Guardar lista"
#undef D_WEB_LIST_DELETE_DONE
#define D_WEB_LIST_DELETE_DONE      "Eliminar elementos marcados"
#undef D_WEB_LIST_HINT
#define D_WEB_LIST_HINT             "Las filas vacías se ignoran. Las filas marcadas se pueden eliminar directamente."

// ----------------------------------------------------------------------------
//  Reset page
// ----------------------------------------------------------------------------
#undef D_WEB_RESET_HEADING
#define D_WEB_RESET_HEADING         "Reinicio de fábrica"
#undef D_WEB_RESET_SUBTITLE
#define D_WEB_RESET_SUBTITLE        "Borre todos los libros, marcapáginas, progreso y recursos personalizados."
#undef D_WEB_RESET_CONFIRM_HEADING
#define D_WEB_RESET_CONFIRM_HEADING "Confirmar reinicio"
#undef D_WEB_RESET_WARNING
#define D_WEB_RESET_WARNING         "Esto eliminará TODOS los libros, marcapáginas y progreso de lectura."
#undef D_WEB_RESET_DETAIL
#define D_WEB_RESET_DETAIL          "El sistema de archivos del dispositivo se formateará y los ajustes volverán a los valores predeterminados."
#undef D_WEB_RESET_YES_BUTTON
#define D_WEB_RESET_YES_BUTTON      "Sí, reiniciar"
#undef D_WEB_RESET_COMPLETE_HEADING
#define D_WEB_RESET_COMPLETE_HEADING "Reinicio de fábrica completo"
#undef D_WEB_RESET_COMPLETE_DESC
#define D_WEB_RESET_COMPLETE_DESC   "Todos los libros, marcapáginas, progreso y recursos personalizados fueron eliminados. El dispositivo está ahora en un estado limpio."
#undef D_WEB_GO_HOME_BUTTON
#define D_WEB_GO_HOME_BUTTON        "Ir al inicio"
#undef D_WEB_OPEN_FILES_BUTTON
#define D_WEB_OPEN_FILES_BUTTON     "Abrir archivos"
#undef D_WEB_RESET_SUCCESS_TITLE
#define D_WEB_RESET_SUCCESS_TITLE   "Reinicio completo"
#undef D_WEB_RESET_SUCCESS_SUBTITLE
#define D_WEB_RESET_SUCCESS_SUBTITLE "Pala One se reinició con éxito."
#undef D_WEB_RESET_BANNER
#define D_WEB_RESET_BANNER          "&#10003; Reinicio de fábrica completo."

// ----------------------------------------------------------------------------
//  Settings page
// ----------------------------------------------------------------------------
#undef D_WEB_SETTINGS_TITLE
#define D_WEB_SETTINGS_TITLE        "Ajustes de Pala One"
#undef D_WEB_SETTINGS_SUBTITLE_PREFIX
#define D_WEB_SETTINGS_SUBTITLE_PREFIX "Firmware "
#undef D_WEB_SETTINGS_SUBTITLE_SUFFIX
#define D_WEB_SETTINGS_SUBTITLE_SUFFIX " — página de configuración almacenada directamente en el dispositivo."
#undef D_WEB_SETTINGS_BACK_NAV
#define D_WEB_SETTINGS_BACK_NAV     "&#8592; Inicio"
#undef D_WEB_READING_HEADING
#define D_WEB_READING_HEADING       "Lectura"
#undef D_WEB_FONT_SIZE_LABEL
#define D_WEB_FONT_SIZE_LABEL       "Tamaño de fuente"
#undef D_WEB_FONT_SIZE_8
#define D_WEB_FONT_SIZE_8           "8px &mdash; diminuto"
#undef D_WEB_FONT_SIZE_10
#define D_WEB_FONT_SIZE_10          "10px &mdash; pequeño"
#undef D_WEB_FONT_SIZE_12
#define D_WEB_FONT_SIZE_12          "12px &mdash; mediano"
#undef D_WEB_FONT_SIZE_14
#define D_WEB_FONT_SIZE_14          "14px &mdash; grande"
#undef D_WEB_FONT_SIZE_HINT
#define D_WEB_FONT_SIZE_HINT        "Controla cuántas líneas caben en cada página."
#undef D_WEB_SLEEP_AFTER_LABEL
#define D_WEB_SLEEP_AFTER_LABEL     "Suspender después de"
#undef D_WEB_SLEEP_30S
#define D_WEB_SLEEP_30S             "30 segundos"
#undef D_WEB_SLEEP_1M
#define D_WEB_SLEEP_1M              "1 minuto"
#undef D_WEB_SLEEP_2M
#define D_WEB_SLEEP_2M              "2 minutos"
#undef D_WEB_SLEEP_5M
#define D_WEB_SLEEP_5M              "5 minutos"
#undef D_WEB_SLEEP_10M
#define D_WEB_SLEEP_10M             "10 minutos"
#undef D_WEB_SLEEP_30M
#define D_WEB_SLEEP_30M             "30 minutos"
#undef D_WEB_SLEEP_HINT
#define D_WEB_SLEEP_HINT            "La suspensión automática mantiene bajo el consumo de batería en reposo."
#undef D_WEB_LINE_SPACING_LABEL
#define D_WEB_LINE_SPACING_LABEL    "Espaciado de línea"
#undef D_WEB_LINE_SPACING_0
#define D_WEB_LINE_SPACING_0        "0 px &mdash; compacto"
#undef D_WEB_LINE_SPACING_1
#define D_WEB_LINE_SPACING_1        "1 px &mdash; normal"
#undef D_WEB_LINE_SPACING_2
#define D_WEB_LINE_SPACING_2        "2 px &mdash; relajado"
#undef D_WEB_LINE_SPACING_3
#define D_WEB_LINE_SPACING_3        "3 px &mdash; suelto"
#undef D_WEB_LINE_SPACING_HINT
#define D_WEB_LINE_SPACING_HINT     "Un pequeño cambio aquí puede facilitar mucho la lectura."
#undef D_WEB_NO_SCREENSAVER_LABEL
#define D_WEB_NO_SCREENSAVER_LABEL  "Modo sin salvapantallas"
#undef D_WEB_NO_SCREENSAVER_HINT
#define D_WEB_NO_SCREENSAVER_HINT   "El dispositivo sigue durmiéndose según el temporizador habitual y actualiza la pantalla antes de dormirse. Luego muestra la última página del libro y omite la actualización completa al despertar, para que puedas continuar leyendo con un solo clic sin la interrupción de un refresco de pantalla."
#undef D_WEB_SAVE_SETTINGS_BUTTON
#define D_WEB_SAVE_SETTINGS_BUTTON  "Guardar ajustes"
#undef D_WEB_SETTINGS_NO_EXTRAS
#define D_WEB_SETTINGS_NO_EXTRAS    "Sin archivos extra, scripts ni fuentes."
#undef D_WEB_SCREENSAVER_HEADING
#define D_WEB_SCREENSAVER_HEADING   "Salvapantallas"
#undef D_WEB_SCREENSAVER_SPECS
#define D_WEB_SCREENSAVER_SPECS     "Suba bytes XBM en bruto: <b>3904 bytes</b>, 250&times;122 px, 1 bit, LSB primero, 32 bytes por fila."
#undef D_WEB_SCREENSAVER_TIP
#define D_WEB_SCREENSAVER_TIP       "Consejo: use <a class='link' href='https://javl.github.io/image2cpp/' target='_blank'>image2cpp</a> con <b>Plain bytes</b>. Invierta los colores si es necesario."
#undef D_WEB_SCREENSAVER_ACTIVE
#define D_WEB_SCREENSAVER_ACTIVE    "&#10003; Salvapantallas personalizado activo."
#undef D_WEB_CONFIRM_DEL_SCREENSAVER
#define D_WEB_CONFIRM_DEL_SCREENSAVER "¿Eliminar salvapantallas personalizado?"
#undef D_WEB_SCREENSAVER_DEFAULT
#define D_WEB_SCREENSAVER_DEFAULT   "Usando salvapantallas predeterminado."
#undef D_WEB_SLEEP_IMAGE_LABEL
#define D_WEB_SLEEP_IMAGE_LABEL     "Archivo de imagen de suspensión"
#undef D_WEB_SCREENSAVER_UPLOAD_BUTTON
#define D_WEB_SCREENSAVER_UPLOAD_BUTTON "Subir imagen"

// Buttons / remappable hold-gestures section.
#undef D_WEB_BUTTONS_HEADING
#define D_WEB_BUTTONS_HEADING       "Botones"
#undef D_WEB_BUTTONS_HINT
#define D_WEB_BUTTONS_HINT          "1 clic = siguiente, 2 = anterior, 3 = inicio. Las tres pulsaciones largas abajo son reasignables."
#undef D_WEB_BUTTONS_LONG
#define D_WEB_BUTTONS_LONG          "Pulsación larga"
#undef D_WEB_BUTTONS_EXTRA_LONG
#define D_WEB_BUTTONS_EXTRA_LONG    "Pulsación muy larga"
#undef D_WEB_BUTTONS_CLICK_HOLD
#define D_WEB_BUTTONS_CLICK_HOLD    "Clic y mantener"
#undef D_WEB_BUTTONS_SAVE
#define D_WEB_BUTTONS_SAVE          "Guardar botones"
#undef D_WEB_BUTTONS_LOCK_HINT
#define D_WEB_BUTTONS_LOCK_HINT     "Si está bloqueado, repita cualquier pulsación larga para desbloquear."
#undef D_WEB_BUTTONS_ACTION_NONE
#define D_WEB_BUTTONS_ACTION_NONE     "Ninguna"
#undef D_WEB_BUTTONS_ACTION_BOOKMARK
#define D_WEB_BUTTONS_ACTION_BOOKMARK "Marcar página"
#undef D_WEB_BUTTONS_ACTION_LOCK
#define D_WEB_BUTTONS_ACTION_LOCK     "Bloquear dispositivo"
#undef D_WEB_BUTTONS_ACTION_MENU
#define D_WEB_BUTTONS_ACTION_MENU     "Abrir menú"

// ----------------------------------------------------------------------------
//  Upload routes
// ----------------------------------------------------------------------------
#undef D_WEB_UPLOAD_COMPLETE_HEADING
#define D_WEB_UPLOAD_COMPLETE_HEADING "Subida completa"
#undef D_WEB_UPLOAD_COMPLETE_DESC
#define D_WEB_UPLOAD_COMPLETE_DESC  "Su libro está ahora almacenado en el dispositivo y disponible en la biblioteca."
#undef D_WEB_UPLOAD_BOOK_LABEL
#define D_WEB_UPLOAD_BOOK_LABEL     "Libro"
#undef D_WEB_UPLOAD_STORED_SIZE
#define D_WEB_UPLOAD_STORED_SIZE    "Tamaño almacenado"
#undef D_WEB_UPLOAD_BOOKS_NOW
#define D_WEB_UPLOAD_BOOKS_NOW      "Libros ahora"
#undef D_WEB_UPLOAD_FREE_SPACE
#define D_WEB_UPLOAD_FREE_SPACE     "Espacio libre"
#undef D_WEB_UPLOAD_ANOTHER
#define D_WEB_UPLOAD_ANOTHER        "Subir otro"
#undef D_WEB_UPLOAD_BOOK_SAVED
#define D_WEB_UPLOAD_BOOK_SAVED     "Libro guardado con éxito."
#undef D_WEB_UPLOAD_FINISHED
#define D_WEB_UPLOAD_FINISHED       "&#10003; Subida finalizada."
#undef D_WEB_UPLOAD_ERR_FALLBACK
#define D_WEB_UPLOAD_ERR_FALLBACK   "Subida fallida"
#undef D_WEB_ERR_LIBRARY_FULL
#define D_WEB_ERR_LIBRARY_FULL      "Biblioteca llena"
#undef D_WEB_ERR_NOT_ENOUGH_SPACE
#define D_WEB_ERR_NOT_ENOUGH_SPACE  "Espacio insuficiente"
#undef D_WEB_ERR_CANT_CREATE_TEMP_BOOK
#define D_WEB_ERR_CANT_CREATE_TEMP_BOOK "No se pudo crear archivo temporal de subida"
#undef D_WEB_ERR_WRITE_FAILED
#define D_WEB_ERR_WRITE_FAILED      "Fallo de escritura (¿sin espacio?)"
#undef D_WEB_ERR_FINALIZE_UPLOAD
#define D_WEB_ERR_FINALIZE_UPLOAD   "Fallo al finalizar la subida"
#undef D_WEB_ERR_EMPTY_UPLOAD
#define D_WEB_ERR_EMPTY_UPLOAD      "Subida vacía"
#undef D_WEB_ERR_UPLOAD_ABORTED
#define D_WEB_ERR_UPLOAD_ABORTED    "Subida abortada"
#undef D_WEB_SLEEP_UPLOAD_ERR_FALLBACK
#define D_WEB_SLEEP_UPLOAD_ERR_FALLBACK "Fallo al subir imagen de suspensión"
#undef D_WEB_SLEEP_UPLOAD_HEADING
#define D_WEB_SLEEP_UPLOAD_HEADING  "Salvapantallas actualizado"
#undef D_WEB_SLEEP_UPLOAD_DESC
#define D_WEB_SLEEP_UPLOAD_DESC     "Su imagen de suspensión personalizada se guardó con éxito y se mostrará la próxima vez que el dispositivo se suspenda."
#undef D_WEB_BACK_TO_SETTINGS
#define D_WEB_BACK_TO_SETTINGS      "Volver a ajustes"
#undef D_WEB_SLEEP_UPLOAD_SUBTITLE
#define D_WEB_SLEEP_UPLOAD_SUBTITLE "Salvapantallas guardado con éxito."
#undef D_WEB_SLEEP_UPLOAD_BANNER
#define D_WEB_SLEEP_UPLOAD_BANNER   "&#10003; Imagen de suspensión personalizada subida."
#undef D_WEB_SLEEP_ERR_TEMP
#define D_WEB_SLEEP_ERR_TEMP        "No se pudo crear archivo temporal de suspensión"
#undef D_WEB_SLEEP_ERR_SIZE
#define D_WEB_SLEEP_ERR_SIZE        "La imagen de suspensión debe ser exactamente 3904 bytes"
#undef D_WEB_SLEEP_ERR_SAVE
#define D_WEB_SLEEP_ERR_SAVE        "Fallo al guardar imagen de suspensión"
#undef D_WEB_SLEEP_ERR_ABORTED
#define D_WEB_SLEEP_ERR_ABORTED     "Subida de imagen de suspensión abortada"

// ----------------------------------------------------------------------------
//  App upload route
// ----------------------------------------------------------------------------
#undef D_WEB_APP_UPLOAD_ERR_FALLBACK
#define D_WEB_APP_UPLOAD_ERR_FALLBACK "Subida de app fallida"
#undef D_WEB_APP_INSTALLED_HEADING
#define D_WEB_APP_INSTALLED_HEADING "App instalada"
#undef D_WEB_APP_INSTALLED_DESC
#define D_WEB_APP_INSTALLED_DESC    "Abra el dispositivo, vaya a <b>Apps</b> en la biblioteca y haga doble clic para ejecutarla."
#undef D_WEB_APP_LABEL
#define D_WEB_APP_LABEL             "App"
#undef D_WEB_APPS_NOW
#define D_WEB_APPS_NOW              "Apps ahora"
#undef D_WEB_APP_INSTALLED_SUBTITLE
#define D_WEB_APP_INSTALLED_SUBTITLE "App guardada en /apps/."
#undef D_WEB_APP_INSTALLED_BANNER
#define D_WEB_APP_INSTALLED_BANNER  "&#10003; App lista para ejecutarse."
#undef D_WEB_APP_VALID_OK
#define D_WEB_APP_VALID_OK          "OK"
#undef D_WEB_APP_VALID_TOO_SMALL
#define D_WEB_APP_VALID_TOO_SMALL   "App inválida (archivo muy pequeño)"
#undef D_WEB_APP_VALID_BAD_MAGIC
#define D_WEB_APP_VALID_BAD_MAGIC   "App inválida (firma incorrecta)"
#undef D_WEB_APP_VALID_BAD_ENTRY
#define D_WEB_APP_VALID_BAD_ENTRY   "App inválida (entrada inválida)"
#undef D_WEB_APP_VALID_BAD_RELOC
#define D_WEB_APP_VALID_BAD_RELOC   "App inválida (tabla reloc inválida)"
#undef D_WEB_APP_VALID_API_FMT
#define D_WEB_APP_VALID_API_FMT     "App inválida (API v%u, requiere v%u)"
#undef D_WEB_APP_VALID_INVALID
#define D_WEB_APP_VALID_INVALID     "App inválida"
#undef D_WEB_APPS_DIR_FULL
#define D_WEB_APPS_DIR_FULL         "Directorio de apps lleno"
#undef D_WEB_ERR_CANT_CREATE_TEMP_APP
#define D_WEB_ERR_CANT_CREATE_TEMP_APP "No se pudo crear archivo temporal de app"
#undef D_WEB_APP_TOO_LARGE
#define D_WEB_APP_TOO_LARGE         "App muy grande (> 48 KB)"
#undef D_WEB_APP_BINARY_TOO_SMALL
#define D_WEB_APP_BINARY_TOO_SMALL  "Binario de app muy pequeño"
#undef D_WEB_APP_CANT_READ_HEADER
#define D_WEB_APP_CANT_READ_HEADER  "No se pudo leer el encabezado de la app"
#undef D_WEB_APP_FINALIZE_FAILED
#define D_WEB_APP_FINALIZE_FAILED   "Fallo al finalizar la subida de la app"
#undef D_WEB_APP_UPLOAD_ABORTED
#define D_WEB_APP_UPLOAD_ABORTED    "Subida de app abortada"

// ----------------------------------------------------------------------------
//  Bookmarks web page
// ----------------------------------------------------------------------------
#undef D_WEB_BOOKMARKS_HEADING
#define D_WEB_BOOKMARKS_HEADING     "Marcapáginas"
#undef D_WEB_BOOKMARKS_SUBTITLE
#define D_WEB_BOOKMARKS_SUBTITLE    "Posiciones de lectura guardadas para Pala One, agrupadas por libro."
#undef D_WEB_NO_BOOKS_YET
#define D_WEB_NO_BOOKS_YET          "Aún no hay libros disponibles."
#undef D_WEB_NO_BOOKMARKS_CARD
#define D_WEB_NO_BOOKMARKS_CARD     "Sin marcapáginas"
#undef D_WEB_BOOKMARKS_OPEN_FAILED_CARD
#define D_WEB_BOOKMARKS_OPEN_FAILED_CARD "Error al abrir"
#undef D_WEB_BOOKMARK_PILL_PREFIX
#define D_WEB_BOOKMARK_PILL_PREFIX  "Marcapáginas "
#undef D_WEB_BOOKMARK_VIEW
#define D_WEB_BOOKMARK_VIEW         "Ver"
#undef D_WEB_CONFIRM_DELETE_BOOKMARK
#define D_WEB_CONFIRM_DELETE_BOOKMARK "¿Eliminar marcapáginas?"
#undef D_WEB_BOOKMARK_DOWNLOAD_ALL
#define D_WEB_BOOKMARK_DOWNLOAD_ALL "Descargar todos los marcapáginas"
#undef D_WEB_BOOKMARK_VIEW_HEADING
#define D_WEB_BOOKMARK_VIEW_HEADING "Vista de marcapáginas"
#undef D_WEB_BOOKMARK_VIEW_SUBTITLE
#define D_WEB_BOOKMARK_VIEW_SUBTITLE "Previsualice el texto de página guardado para este marcapáginas."
#undef D_WEB_BOOKMARK_VIEW_BACK_NAV
#define D_WEB_BOOKMARK_VIEW_BACK_NAV "&#8592; Atrás"
#undef D_WEB_BOOKMARK_PAGE_EMPTY
#define D_WEB_BOOKMARK_PAGE_EMPTY   "(vacío)"
#undef D_WEB_BOOKMARK_OPEN_FAILED_DOT
#define D_WEB_BOOKMARK_OPEN_FAILED_DOT "Error al abrir."

// Bookmark export plaintext labels
#undef D_WEB_BMEXPORT_BOOK
#define D_WEB_BMEXPORT_BOOK         "Libro: "
#undef D_WEB_BMEXPORT_BOOKMARKS
#define D_WEB_BMEXPORT_BOOKMARKS    "Marcapáginas: "
#undef D_WEB_BMEXPORT_BOOKMARK_LBL
#define D_WEB_BMEXPORT_BOOKMARK_LBL "Marcapáginas "
#undef D_WEB_NO_BOOKMARKS_THIS_BOOK
#define D_WEB_NO_BOOKMARKS_THIS_BOOK "Sin marcapáginas para este libro"

// ----------------------------------------------------------------------------
//  Lector en el navegador + buscar/saltar (src/web/find.cpp).
// ----------------------------------------------------------------------------
#undef D_WEB_READ_TITLE
#define D_WEB_READ_TITLE            "Leer"
#undef D_WEB_READ_SUBTITLE
#define D_WEB_READ_SUBTITLE         "Explora y busca el libro en tu navegador. Usa Saltar para fijar el punto de retoma del dispositivo."
#undef D_WEB_READ_BYTES_LABEL
#define D_WEB_READ_BYTES_LABEL      "bytes"
#undef D_WEB_READ_CURRENT_PAGE_LABEL
#define D_WEB_READ_CURRENT_PAGE_LABEL "página actual:"
#undef D_WEB_READ_FIND_PLACEHOLDER
#define D_WEB_READ_FIND_PLACEHOLDER "Buscar en el libro"
#undef D_WEB_READ_FIND_ALL
#define D_WEB_READ_FIND_ALL         "Buscar todo"
#undef D_WEB_READ_FIND_PREV
#define D_WEB_READ_FIND_PREV        "Anterior"
#undef D_WEB_READ_FIND_NEXT
#define D_WEB_READ_FIND_NEXT        "Siguiente"
#undef D_WEB_READ_JUMP_HERE
#define D_WEB_READ_JUMP_HERE        "Saltar aquí"
#undef D_WEB_READ_LOADING
#define D_WEB_READ_LOADING          "Cargando texto del libro..."
#undef D_WEB_READ_PAGE_PLACEHOLDER
#define D_WEB_READ_PAGE_PLACEHOLDER "Número de página"
#undef D_WEB_READ_JUMP_PAGE
#define D_WEB_READ_JUMP_PAGE        "Saltar a página"
#undef D_WEB_READ_JUMP_HINT
#define D_WEB_READ_JUMP_HINT        "Guarda directamente la próxima página de apertura."
#undef D_WEB_READ_AND_FIND_LINK
#define D_WEB_READ_AND_FIND_LINK    "Leer y buscar"

// ----------------------------------------------------------------------------
//  Familia de fuente + lectura biónica + retención de posición
//  (src/web/settings.cpp).
// ----------------------------------------------------------------------------
#undef D_WEB_READING_INTRO
#define D_WEB_READING_INTRO         "Cambiar la fuente, familia, espaciado de línea o modo biónico mantiene tu lugar en el libro actual &mdash; el dispositivo reorganiza las páginas alrededor del byte que estás leyendo y aterriza en la página que lo contiene."
#undef D_WEB_FONT_FAMILY_LABEL
#define D_WEB_FONT_FAMILY_LABEL     "Familia de fuente"
#undef D_WEB_FONT_FAMILY_HELVETICA
#define D_WEB_FONT_FAMILY_HELVETICA "Helvetica"
#undef D_WEB_FONT_FAMILY_DYSLEXIC
#define D_WEB_FONT_FAMILY_DYSLEXIC  "OpenDyslexic"
#undef D_WEB_FONT_FAMILY_HINT
#define D_WEB_FONT_FAMILY_HINT      "OpenDyslexic usa formas de letra más gruesas diseñadas para una lectura más fácil."
#undef D_WEB_BIONIC_LABEL
#define D_WEB_BIONIC_LABEL          "Lectura biónica"
#undef D_WEB_BIONIC_HINT
#define D_WEB_BIONIC_HINT           "Resalta en negrita las primeras letras de cada palabra para anclar la mirada."
#undef D_WEB_SETTINGS_APPLY_HINT
#define D_WEB_SETTINGS_APPLY_HINT   "Los cambios se aplican en la próxima página renderizada."

// ----------------------------------------------------------------------------
//  Tarjeta de salvapantallas en la página de ajustes (src/web/settings.cpp).
// ----------------------------------------------------------------------------
#undef D_WEB_SCREENSAVER_CARD_DESC
#define D_WEB_SCREENSAVER_CARD_DESC "Administra la imagen (o rotación de imágenes) que se muestra en la pantalla cuando el dispositivo se suspende."
#undef D_WEB_SCREENSAVER_EDITOR_LINK
#define D_WEB_SCREENSAVER_EDITOR_LINK "Abrir editor de salvapantallas"
#undef D_WEB_SCREENSAVER_EDITOR_HINT
#define D_WEB_SCREENSAVER_EDITOR_HINT "Incluye un editor de bitmap en el navegador y hasta 8 ranuras de rotación."

// ----------------------------------------------------------------------------
//  Editor y administrador multi-ranura de salvapantallas (src/web/screensavers.cpp).
//  Las cadenas internas del editor en JS (estado / errores) aún NO están i18n'd.
// ----------------------------------------------------------------------------
#undef D_WEB_SS_TITLE
#define D_WEB_SS_TITLE              "Salvapantallas"
#undef D_WEB_SS_SUBTITLE
#define D_WEB_SS_SUBTITLE           "Imágenes de suspensión personalizadas, rotación multi-ranura y editor de bitmap en el firmware."
#undef D_WEB_SS_ROTATION_HEADING
#define D_WEB_SS_ROTATION_HEADING   "Rotación"
#undef D_WEB_SS_ROTATION_INTRO
#define D_WEB_SS_ROTATION_INTRO     "Elige qué se muestra en la pantalla cuando el dispositivo se suspende. Cycle recorre las ranuras pobladas en orden; Shuffle elige al azar sin repeticiones inmediatas."
#undef D_WEB_SS_MODE_LABEL
#define D_WEB_SS_MODE_LABEL         "Modo"
#undef D_WEB_SS_MODE_SINGLE
#define D_WEB_SS_MODE_SINGLE        "Solo una imagen"
#undef D_WEB_SS_MODE_CYCLE
#define D_WEB_SS_MODE_CYCLE         "Ciclar entre ranuras"
#undef D_WEB_SS_MODE_SHUFFLE
#define D_WEB_SS_MODE_SHUFFLE       "Mezclar ranuras"
#undef D_WEB_SS_SLOTS_POPULATED
#define D_WEB_SS_SLOTS_POPULATED    "Ranuras pobladas: "
#undef D_WEB_SS_SAVE_MODE
#define D_WEB_SS_SAVE_MODE          "Guardar modo"
#undef D_WEB_SS_SLOTS_HEADING
#define D_WEB_SS_SLOTS_HEADING      "Ranuras de rotación"
#undef D_WEB_SS_SLOT_LABEL
#define D_WEB_SS_SLOT_LABEL         "Ranura"
#undef D_WEB_SS_SLOT_EMPTY
#define D_WEB_SS_SLOT_EMPTY         "vacía"
#undef D_WEB_SS_CONFIRM_DEL_SLOT
#define D_WEB_SS_CONFIRM_DEL_SLOT   "¿Eliminar esta ranura?"
#undef D_WEB_SS_DOWNLOAD_ARIA
#define D_WEB_SS_DOWNLOAD_ARIA      "Descargar salvapantallas"
#undef D_WEB_SS_DELETE_ARIA
#define D_WEB_SS_DELETE_ARIA        "Eliminar salvapantallas"
#undef D_WEB_SS_ROTATE
#define D_WEB_SS_ROTATE             "Girar 90\u00b0"
#undef D_WEB_SS_SINGLE_HEADING
#define D_WEB_SS_SINGLE_HEADING     "Salvapantallas único"
#undef D_WEB_SS_SINGLE_ALT
#define D_WEB_SS_SINGLE_ALT         "Salvapantallas único"
#undef D_WEB_SS_CONFIRM_DEL_SINGLE
#define D_WEB_SS_CONFIRM_DEL_SINGLE "¿Eliminar el salvapantallas único?"
#undef D_WEB_SS_NO_SINGLE
#define D_WEB_SS_NO_SINGLE          "Sin salvapantallas único cargado."
#undef D_WEB_SS_EDITOR_HEADING
#define D_WEB_SS_EDITOR_HEADING     "Editor"
#undef D_WEB_SS_EDITOR_INTRO
#define D_WEB_SS_EDITOR_INTRO       "Arrastra una imagen al editor y súbela a una ranura de rotación o como el salvapantallas único heredado. Todas las imágenes se renderizan en 250&times;122 1 bit (3904 bytes)."
#undef D_WEB_SS_SOURCE_IMAGE
#define D_WEB_SS_SOURCE_IMAGE       "Imagen fuente"
#undef D_WEB_SS_TOLERANCE
#define D_WEB_SS_TOLERANCE          "Tolerancia de negro"
#undef D_WEB_SS_INVERT
#define D_WEB_SS_INVERT             "Invertir blanco/negro"
#undef D_WEB_SS_PRECISE_CONTROL
#define D_WEB_SS_PRECISE_CONTROL    "Control preciso"
#undef D_WEB_SS_ZOOM
#define D_WEB_SS_ZOOM               "Zoom"
#undef D_WEB_SS_MOVE_X
#define D_WEB_SS_MOVE_X             "Mover X"
#undef D_WEB_SS_MOVE_Y
#define D_WEB_SS_MOVE_Y             "Mover Y"
#undef D_WEB_SS_PREVIEW_LABEL
#define D_WEB_SS_PREVIEW_LABEL      "Vista previa (arrastra para mover, pellizca o usa la rueda para zoom)"
#undef D_WEB_SS_RESET_FIT
#define D_WEB_SS_RESET_FIT          "Reiniciar ajuste"
#undef D_WEB_SS_NO_IMAGE
#define D_WEB_SS_NO_IMAGE           "Sin imagen cargada"
#undef D_WEB_SS_SAVE_TO
#define D_WEB_SS_SAVE_TO            "Guardar en"
#undef D_WEB_SS_DST_SINGLE
#define D_WEB_SS_DST_SINGLE         "Salvapantallas único (/sleep.bin)"
#undef D_WEB_SS_DST_AUTO_PREFIX
#define D_WEB_SS_DST_AUTO_PREFIX    "Próxima ranura libre (ranura "
#undef D_WEB_SS_DST_AUTO_SUFFIX
#define D_WEB_SS_DST_AUTO_SUFFIX    ")"
#undef D_WEB_SS_DST_FULL
#define D_WEB_SS_DST_FULL           "(Todas las ranuras llenas)"
#undef D_WEB_SS_DST_SLOT_PREFIX
#define D_WEB_SS_DST_SLOT_PREFIX    "Ranura de rotación "
#undef D_WEB_SS_DST_OVERWRITE
#define D_WEB_SS_DST_OVERWRITE      " (sobrescribir)"
#undef D_WEB_SS_UPLOAD_EDITED
#define D_WEB_SS_UPLOAD_EDITED      "Subir imagen editada"

#endif  // PALA_LANG_ES_LA_H
